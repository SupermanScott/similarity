<?php
// $Id: similarity.inc,v 1.1.2.10 2009/06/03 07:04:18 scottreynolds Exp $

/**
 * @file similarity.inc
 * lovingly crafted by Scott Reynolds
 *
 * Provides the generic factory methods for loading a similarity object and outlines
 * out the abstract classes for node similarities and similarities in general
 */

/**
 * A Similarity object that consists of conditions, tokenizer, transformers and
 * an algorithm.
 */
class Similarity {
  // The unique name of the object.
  public $machine_name;

  // The configuration options are stored in here.
  public $options = array();

  // Whether or not this object resides in the object_cache.
  public $from_cache = FALSE;

  // Candidates for the calculations.
  protected $candidates = NULL;

  // Base object.
  protected $ctools_base_object;

  public function __construct($ctools_object) {
    $this->machine_name = $ctools_object->machine_name;
    $this->options = similarity_compile_options($ctools_object->options, similarity_compress_option_definition($this->optionsDefinition()));

    $this->ctools_base_object = $ctools_object;
    ctools_include('plugins');
  }

  protected function optionsDefinition() {
    return array(
      'title' => array('#default_value' => ''),
      'description' => array('#default_value' => ''),
      'min_sim' => array('#default_value' => 0.50),
      'max_time' => array('#default_value' => 15),
      'base' => array('#default_value' => 'node'),
      'base_field' => array('#default_value' => 'nid'),
      'conditions' => array('#default_value' => array()),
      'tokenizers' => array('#default_value' => array()),
      'transformers' => array('#default_value' => array()),
      'algorithms' => array(
        'plugin_id' => array('#default_value' => 'similarity_cosine'),
        'options' => array(
          '#default_value' => array(),
        ),
      ),
    );
  }

  /**
   * Present the configurable options.
   */
  public function optionForm($form_state) {
    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('Human readable title of the similarity object. This name will be visible only on Adminstration pages'),
      '#required' => TRUE,
      '#default_value' => $this->options['title'],
      '#size' => 40,
    );
    $form['description'] = array(
      '#type' => 'textarea',
      '#title' => t('Description'),
      '#description' => t('Description about what this similarity does.'),
      '#default_value' => $this->options['description'],
    );
    $form['min_sim'] = array(
      '#type' => 'select',
      '#title' => t('Minimum similarity score'),
      '#options' => drupal_map_assoc(range(0, 100, 5)),
      '#description' => t('Only store similarities greater then this number. This means that if a node is similar to another node to the 50%, it will only be stored if this value is 50 or greater'),
      '#default_value' => $this->options['min_sim'] * 100,
      '#element_validate' => array('similarity_validate_min_sim'),
    );
    $form['max_time'] = array(
      '#type' => 'select',
      '#title' => t('Maximum calculation time'),
      '#description' => t('The maximum amount of time in seconds spent on Drupal cron run calculating similarities for this object.'),
      '#options' => drupal_map_assoc(range(5, 60, 5)),
      '#default_value' => $this->options['max_time'],
    );

    return $form;
  }

  /**
   * Return all of the tokens for the given object_id
   *
   * @param $object_id
   *  The unique id for the object of type $this->config['base']
   *
   * @return array
   *  A keyed array with 'term' as the key and the value is its relevancy.
   */
  protected function tokenize($object_id) {
    $tokens = array();

    $cached_data = cache_get($this->machine_name . ':' . $object_id, 'cache_similarity_tokens');
    if (!empty($cached_data)) {
      return $cached_data->data;
    }

    foreach ($this->options['tokenizers'] as $tokenizer_definition) {
      $tokenizer_function = ctools_plugin_load_function('similarity', 'tokenizers', $tokenizer_definition['plugin_id'], 'callback');
      if (!empty($tokenizer_function)) {
        // Tokens can be overridden in the case that both tokenizers return the
        // same words, the last one will win out.
        $tokens += $tokenizer_function($object_id, $tokenizer_definition['options'], $this->options['base']);
      }
    }

    foreach ($this->options['transformers'] as $transformer_definition) {
      $transformer = ctools_plugin_load_function('similarity', 'transformers', $transformer_definition['plugin_id'], 'callback');
      if ($transformer) {
        // Transformers turn the tokens from the tokenizers into something else.
        $tokens = $transformer($object_id, $tokens, $transformer_definition['options']);
      }
    }

    cache_set($this->machine_name . ':' . $object_id, $tokens, 'cache_similarity_tokens');
    return $tokens;
  }

  /**
   * Calculate the similarities from object_id to every other object_id in set.
   *
   * @param $object_id
   *  The object_id for the basis of this calculation.
   *
   * @param $object_set
   *  All the other object_ids to compare.
   */
  public function calculate($object_id, $object_set) {
    $base_tokens = $this->tokenize($object_id);
    $algorithm = ctools_plugin_load_function('similarity', 'algorithms', $this->options['algorithms']['plugin_id'], 'callback');

    foreach ($object_set as $other_id) {
      if ($other_id != $object_id) {
        $other_tokens = $this->tokenize($other_id);
        $similarity = $algorithm($base_tokens, $other_tokens, $this->options['algorithms']['options']);
        if ($similarity >= $this->options['min_sim']) {
          $this->saveSimilarity($object_id, $other_id, $similarity);
        }
      }
    }
  }

  /**
   * Save the similarity away.
   *
   * @param $object_id1
   *  The id of the first object.
   *
   * @param $object_id2
   *  The id fo the second object.
   *
   * @param $similarity
   *  The similarity score.
   */
  protected function saveSimilarity($object_id1, $object_id2, $similarity) {
    if (!db_table_exists('similarity_' . $this->machine_name)) {
      $ret = array();
      db_create_table($ret, 'similarity_' . $this->machine_name, $this->schema());
    }

    db_query('UPDATE {similarity_' . $this->machine_name . '} SET score = %f, created = %d WHERE nid1 = %d AND nid2 = %d', $similarity, time(), $object_id1, $object_id2);
    if (!db_affected_rows()) {
      db_query('INSERT INTO {similarity_' . $this->machine_name . '} (score, created, nid1, nid2) VALUE (%f, %d, %d, %d)', $similarity, time(), $object_id1, $object_id2);
    }

    db_query('UPDATE {similarity_' . $this->machine_name . '} SET score = %f, created = %d WHERE nid1 = %d AND nid2 = %d', $similarity, time(), $object_id2, $object_id1);
    if (!db_affected_rows()) {
      db_query('INSERT INTO {similarity_' . $this->machine_name . '} (score, created, nid1, nid2) VALUE (%f, %d, %d, %d)', $similarity, time(), $object_id2, $object_id1);
    }
  }

  /**
   * Find all the candidate base_fields
   *
   * @return array
   *  an array for $this->base_field ids.
   */
  public function findCandidates() {
    // Use the property $candidates as a static cache.
    if (!isset($this->candidates)) {
      $query = array(
        'filters' => array(),
        'arguments' => array(),
        'joins' => array(),
      );

      foreach ($this->options['conditions'] as $condition_definition) {
        $condition_function = ctools_plugin_load_function('similarity', 'conditions', $condition_definition['plugin_id'], 'callback');
        if ($condition_function) {
          $query = $condition_function($query, $condition_definition['options']);
        }
      }

      // Build the query based on the $query array.
      $query_string = "SELECT {" . $this->options['base'] . "}." . $this->options['base_field'] . " FROM {" . $this->options['base'] . "}";
      if (!empty($query['joins'])) {
        $query_string .= " " . implode(" ", $query['joins']);
      }

      if (!empty($query['filters'])) {
        $query_string .= " WHERE " . implode(" AND ", $query['filters']);
      }

      $query_string .= " ORDER BY {" . $this->options['base'] . "}." . $this->options['base_field'] . " ASC";

      $result = db_query($query_string, $query['arguments']);
      $ids = array();
      while ($row = db_fetch_object($result)) {
        $ids[] = $row->{$this->options['base_field']};
      }

      $this->candidates = $ids;
    }
    return $this->candidates;
  }

  /**
   * Reset the candidates cache.
   */
  public function resetCandidates() {
    $this->candidates = NULL;
  }

  /**
   * Returns all the ids that need to be reindexed.
   *
   * @param $id
   *  The id that has just been changed.
   */
  public function updateIndex($id) {
    $ids = array();
    foreach ($this->options['tokenizers'] as $tokenizer_definition) {
      $reindex_function = ctools_plugin_load_function('similarity', 'tokenizers', $tokenizer_definition['plugin_id'], 'reindex_candidates_callback');

      if (!empty($reindex_function)) {
        $ids = array_merge($ids, $reindex_function($id, $tokenizer_definition['options']));
      }
    }

    // Because the tokenizers have no idea about the conditions, leverage the
    // conditions here to find the union of the two sets.
    return array_intersect($ids, $this->findCandidates());
  }

  /**
   * Return the title.
   */
  public function title() {
    return $this->options['title'];
  }

  /**
   * Return the description.
   */
  public function description() {
    return $this->options['description'];
  }

  /**
   * Provide the schema definition for this object.
   */
  protected function schema() {
    $schema = array(
      'fields' => array(
        'nid1' => array(
          'type' => 'int',
          'size' => 'normal',
          'not null' => TRUE,
        ),
        'nid2' => array(
          'type' => 'int',
          'size' => 'normal',
          'not null' => TRUE,
        ),
        'score' => array(
          'type' => 'float',
          'not null' => TRUE,
        ),
        'created' => array(
          'descritpion' => t('Used as a timestamp from when the entry was last created'),
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      'primary key' => array('nid1', 'nid2'),
      'indexes' => array(
        'id_1' => array('nid1'),
        'id_2' => array('nid2'),
      ),
    );
    
    return $schema;
  }

  /**
   * Provide the Views data for this similarity.
   */
  public function viewsData() {
    $table_name = 'similarity_' . $this->machine_name;
    
    $data[$table_name]['table']['group'] = t('Similarity');
    $data[$table_name]['table']['join'] = array(
      'node' => array(
        'left_field' => 'nid',
        'field' => 'nid1',
        'type' => 'INNER',
      ),
    );
    $data[$table_name]['nid2'] = array(
      'title' => t('@name Similary NID', array('@name' => $this->title())),
      'argument' => array(
        'handler' => 'views_handler_argument_node_nid',
      ),
      'help' => t('Node id of the similar node'),
    );
    
    $data[$table_name]['score'] = array(
      'title' => t('@name Similarity score', array('@name' => $this->title())),
      'field' => array(
        'handler' => 'views_handler_field_numeric', // TODO: write this handler
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'help' => t('Similarity score of the second node'),
    );

    return $data;
  }

  public function exportObject() {
    // @TODO: remove the defaulted options so it reduces the export.
    if ($this->from_cache) {
      $this->ctools_base_object->options = $this->options;
    }
    return $this->ctools_base_object;
  }

  /**
   * Generic get function to return ctools object properties.
   */
  public function __get($property) {
    if (isset($this->ctools_base_object->$property)) {
      return $this->ctools_base_object->$property;
    }
    return NULL;
  }
}