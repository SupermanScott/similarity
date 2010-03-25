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
  protected $options = array();

  public function __construct($name, $options) {
    $this->machine_name = $name;
    $this->options = $this->compileOptions($options, $this->defaultOptions($this->optionsDefinition()));

    ctools_include('plugins');
  }

  protected function optionsDefinition() {
    return array(
      'title' => array('#default_value' => ''),
      'min_sim' => array('#default_value' => 0.50),
      'time_limit' => array('#default_value' => 15),
      'base' => array('#default_value' => 'node'),
      'base_field' => array('#default_value' => 'nid'),
      'conditions' => array('#default_value' => array()),
      'tokenizer' => array(
        'callback_id' => array('#default_value' => ''),
        'options' => array('#default_value' => array()),
      ),
      'transformers' => array('#default_value' => array()),
      'algorithm' => array(
        'callback_id' => array('#default_value' => 'similarity_cosine'),
        'options' => array(
          '#default_value' => array(),
        ),
      ),
    );
  }

  /**
   * Take option definition and produces an options array.
   */
  protected function defaultOptions($defaults) {
    $options = array();
    foreach (element_children($defaults) as $key) {
      if (isset($defaults[$key]['#default_value'])) {
        $options[$key] = $defaults[$key]['#default_value'];
      }
      else {
        $options[$key] = $this->defaultOptions($defaults[$key]);
      }
    }

    return $options;
	}

  /**
   * Set the options by respecting the defaults.
   */
  protected function compileOptions($options, $default_options) {
    $compiled_options = array();

    // If there isn't a default for this, then use the $options as is.
    if (empty($default_options) && !empty($options)) {
      $compiled_options = $options;
    }
    // If there are no more overrides, use the defaults as is
    elseif (empty($options)) {
      $compiled_options = $default_options;
    }
    // Otherwise, compile them together.
    else {
      foreach ($default_options as $key => $option) {
        if (is_array($option)) {
          if (!isset($options[$key])) {
            $options[$key] = array();
          }
          $compiled_options[$key] = $this->compileOptions($options[$key], $default_options[$key]);
        }
        elseif (isset($options[$key]) && !is_array($options[$key])) {
          $compiled_options[$key] = $options[$key];
        }
        else {
          $compiled_options[$key] = $default_options[$key];
        }
      }
    }

    return $compiled_options;
  }

  /**
   * Present the configurable options.
   */
  public function optionForm(&$form, $form_state) {
    $form['title'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('Human readable title of the similarity object. This name will be visible only on Adminstration pages'),
      '#required' => TRUE,
      '#default_value' => $this->options['title'],
      '#size' => 40,
    );
    $form['min_sim'] = array(
      '#type' => 'select',
      '#title' => t('Minimum similarity score'),
      '#options' => drupal_map_assoc(range(50, 100, 5)),
      '#description' => t('Only store similarities greater then this number. This means that if a node is similar to another node to the 50%, it will only be stored if this value is 50 or greater'),
      '#default_value' => $this->options['min_sim'],
    );
    $form['time_limit'] = array(
      '#type' => 'select',
      '#title' => t('Cron time limit'),
      '#description' => t('Number of seconds this object is allowed to do its own processing'),
      '#default_value' => $this->options['time_limit']
    );
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

    $tokenizer_function = ctools_plugin_load_function('similarity', 'tokenizers', $this->options['tokenizer']['callback_id'], 'callback');

    $tokens = $tokenizer_function($object_id, $this->options['tokenizer']['options'], $this->options['base']);

    foreach ($this->options['transformers'] as $transformer_definition) {
      $transformer = ctools_plugin_load_function('similarity', 'transformers', $transformer_definition['callback_id']);
      if ($transformer) {
        $tokens = $transformer($tokens, $transformer_definition['options']);
      }
    }
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
    $algorithm = ctools_plugin_load_function('similarity', 'algorithms', $this->options['algorithm']['callback_id'], 'callback');

    foreach ($object_set as $other_id) {
      if ($other_id >= $object_id) {
        // Only calculate a similarity between the two pairs once. To ensure
        // that happens, only calculate similarities between $object_id and all
        // other ids less then it.
        break;
      }
     
      $other_tokens = $this->tokenize($other_id);
      $similarity = $algorithm($base_tokens, $other_tokens, $this->options['algorithm']['options']);
      if ($similarity >= $this->options['min_sim']) {
        $this->saveSimilarity($object_id, $other_id, $similarity);
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

    // Store nid1 as the smallest id.
    if ($object_id1 < $object_id2) {
      $nid1 = $object_id1;
      $nid2 = $object_id2;
    }
    else {
      $nid1 = $object_id2;
      $nid2 = $object_id1;
    }

    db_query('UPDATE {similarity_' . $this->machine_name . '} SET score = %f, created = %d WHERE nid1 = %d AND nid2 = %d', $similarity, time(), $nid1, $nid2);
    if (!db_affected_rows()) {
      db_query('INSERT INTO {similarity_' . $this->machine_name . '} (score, created, nid1, nid2) VALUE (%f, %d, %d, %d)', $similarity, time(), $nid1, $nid2);
    }
  }

  /**
   * Find all the candidate base_fields
   *
   * @return array
   *  an array for $this->base_field ids.
   */
  public function findCandidates() {
    $query = array(
      'filters' => array(),
      'arguments' => array(),
      'joins' => array(),
    );

    foreach ($this->options['conditions'] as $condition_defintion) {
      $condition_function = ctools_plugin_load_function('similarity', 'conditions', $condition_defintion['callback_id']);
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
    return $ids;
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
  protected function views_data() {
    $table_name = $this->machine_name . '_similarity';
    
    $data[$table_name]['table']['group'] = t('Similarity');
    $data[$table_name]['table']['join'] = array(
      'node' => array(
        'left_field' => 'nid',
        'field' => 'nid1',
        'type' => 'LEFT',
      ),
    );
     $data[$table_name . "_reverse"]['table']['group'] = t('Similarity');
    $data[$table_name . "_reverse"]['table']['join'] = array(
      'node' => array(
        'left_field' => 'nid',
        'field' => 'nid2',
        'type' => 'LEFT',
      ),
    );
    $data[$table_name]['nid1'] = array(
      'title' => t('@name Similary NID', array('@name' => $this->title)),
      'argument' => array(
        'handler' => 'similarity_views_argument_nid',
      ),
      'help' => t('Node id of the similar node'),
    );
    
    $data[$table_name]['score'] = array(
      'title' => t('@name Similarity score', array('@name' => $this->title)),
      'field' => array(
        'handler' => 'similarity_views_handler_field_score', // TODO: write this handler
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'help' => t('Similarity score of the second node'),
    );

    return $data;
  }
}