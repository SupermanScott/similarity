<?php

class similarity_node_term extends similarity {
  private $pending_saves = array();
  
  /**
   * save the similarity away
   */
  protected function save_similarity($tid1, $tid2, $similarity) {
    static $registered = FALSE;
    if (!$registered) {
      register_shutdown_function(array($this, 'write_similarities'));
      $registered = TRUE;
    }
    if ($tid1 > $tid2) {
      $key1 = $tid1;
      $key2 = $tid2;
    }
    else {
      $key1 = $tid2;
      $key2 = $tid1;
    }
    // put it in the pending saves only if it has a close enough similarity
    if (!isset($this->pending_saves[$key1][$key2])) {
      $this->pending_saves[$key1][$key2] = $similarity;
    }
  }
  
  /**
   * Save the similarities away
   */
  public function write_similarities() {
    static $written = FALSE;
    if ($written) return;
    
    $value_string = '';
    $delete_string = '';
    foreach($this->pending_saves as $tid1 => $pair) {
      foreach($pair as $tid2 => $sim) {
        $value_string .= "($tid1, $tid2, $sim), ";
      }
    }
    
    if (!empty($value_string)) {
      // write the big insert string
      db_query("INSERT INTO {" . $this->machine_name . "_similarity} VALUES " . substr($value_string, 0, -2) . " ON DUPLICATE KEY UPDATE score = VALUES(score)");
    }
    $written = TRUE;
    $this->cleanup();
  }
  
  /**
   * provide a table schema
   */
  protected function schema() {
    $schema = array(
      'fields' => array(
        'tid1' => array(
          'type' => 'int',
          'size' => 'normal',
          'not null' => TRUE,
        ),
        'tid2' => array(
          'type' => 'int',
          'size' => 'normal',
          'not null' => TRUE,
        ),
        'score' => array(
          'type' => 'float',
          'not null' => TRUE,
        ),
      ),
      'primary key' => array('tid1', 'tid2'),
    );
    
    return $schema;
  }
  
  /**
   * Implementation of the option form function
   *
   * @return
   * structured array (Drupal form)
   */
  public function options_form() {
    $form = parent::options_form();
    $vocabs = taxonomy_get_vocabularies();
    foreach ($vocabs as $vid => $vocab_obj) {
      $options[$vid] = $vocab_obj->name;
    }
    $form['term_limit'] = array(
      '#type' => 'select',
      '#title' => t('Term Throttle'),
      '#options' => drupal_map_assoc(range(50, 500, 50)),
      '#description' => t('Select the number of term similarities to calculate per cron run'),
      '#default_value' => isset($this->term_limit) ? $this->term_limit : 100,
    );
    $form['vocab'] = array(
      '#title' => t('Vocabularity'),
      '#description' => t('The vocabularity to draw similaries from'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => isset($this->vocab) ? $this->vocab : 0,
    );
    return $form;
  }
  
  /**
   * returns the structure array for its tables
   */
  function views_data() {
    $table_name = $this->machine_name . '_similarity';
    
    $data[$table_name]['table']['group'] = t('Similarity');
    $data[$table_name]['table']['join'] = array(
      'term_data' => array(
        'left_field' => 'tid',
        'field' => 'tid1',
        'handler' => 'similarity_views_join',
        'type' => 'INNER',
      ),
    );
    $data[$table_name]['tid2'] = array(
      'title' => t('@name Similary term id', array('@name' => $this->title)),
      'argument' => array(
        'handler' => 'similarity_views_argument_tid',
      ),
      'help' => t('Node id of the similar node'),
    );
    
    $data[$table_name]['score'] = array(
      'title' => t('@name Similarity score', array('@name' => $this->title)),
      'field' => array(
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'help' => t('Similarity score of the second node'),
    );

    return $data;
  }
  
  /**
   * Override the calculate function
   */
  function calculate() {
    $all_result = db_query("SELECT n.tid, n.nid FROM {term_node} n JOIN term_data d ON d.tid = n.tid WHERE d.vid = %d ORDER BY n.tid", $this->vocab);
    $data = array();
    while ($term_node = db_fetch_object($all_result)) {
      $data[$term_node->tid][$term_node->nid] = 1; // prep for the cosine sim function
    }
    
    $last = variable_get($this->machine_name . '_last_term', 0);
    $tid_result = db_query_range("SELECT d.tid FROM {term_data} d WHERE d.tid > %d ORDER BY d.tid", $last, 0, $this->term_limit);
    while ($term = db_fetch_object($tid_result)) {
      $current_nids = $data[$term->tid];
      $last_tid = $term->tid;
      
      foreach ($data as $other_tid => $other_nids) {
        if ($other_tid != $term->tid) {
          $this->save_similarity($term->tid, $other_tid, similarity_cosine($current_nids, $other_nids));
        }
      }
    }
    $this->write_similarities();
    if ($last_tid >= db_result(db_query("SELECT MAX(tid) FROM {term_data}"))) {
      $last_tid = 0;
    }
    variable_set($this->machine_name . '_last_term', $last_tid);
  }
}