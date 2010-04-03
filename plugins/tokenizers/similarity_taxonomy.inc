<?php
// $Id: $
/**
 * @file:
 * provides a tokenizer plugin that uses the taxonomy terms as a basis.
 */
$plugin = array(
  'title' => t('Taxonomy'),
  'callback' => 'similarity_tokenizer_taxonomy',
  'option_definition' => array(
    'vid' => array(
      '#default_value' => 0,
    ),
  ),
  'form_callback' => 'similarity_tokenizer_taxonomy_form',
  'base' => array('node'),
	'help_topic' => 'tokenizer-taxonomy',
);

/**
 * Taxonomy tokenizer.
 *
 * @param $nid
 *   The node id for these tokens.
 * @param $options
 *   The options provided to this plugin.
 * @return array
 *   a tid => tf/idf array for all terms assigned to the node.
 */
function similarity_tokenizer_taxonomy($nid, $options) {
  static $total_nodes = NULL, $idf = array();
  if ($options['vid'] == 0) {
    $tids = db_query("SELECT tid FROM {term_node} WHERE nid = %d", $nid);
  }
  else {
    $tids = db_query("SELECT tn.tid FROM {term_node} tn INNER JOIN {term_data} td ON td.tid = tn.tid AND td.vid = %d WHERE tn.tid = %d", $options['vid'], $nid);
  }
  $tokens = array();

  if (!isset($total_nodes)) {
    $total_nodes = db_result(db_query("SELECT COUNT(nid) FROM {node}"));
  }

  while ($tid_object = db_fetch_object($tids)) {
    // Calculate IDF.
    if (!isset($idf[$tid_object->tid])) {
      if (module_exists('term_node_count')) {
        $frequency = db_result(db_query("SELECT node_count FROM {term_node_count} WHERE tid = %d", $tid_object->tid));
      }
      else {
        $frequency = db_result(db_query("SELECT COUNT(tid) FROM {term_node} WHERE tid = %d", $tid_object->tid));
      }
      $idf[$tid_object->tid] = $total_nodes / $frequency;
    }

    // The tokens are keyed by 'word' and the value is
    // TF / IDF (term frequency divided by inverse document frequency)
    // Where term frequency is one (a term can only be applied once to a node)
    // and idf is number nodes divided by the number of nodes with this term.
    $tokens[$tid_object->tid] = 1 / $idf[$tid_object->tid];
  }

  return $tokens;
}

/**
 * FAPI configuration form for the taxonomy tokenizer.
 */
function similarity_tokenizer_taxonomy_form($form_state, $options) {
  foreach(taxonomy_get_vocabularies() as $vid => $vocab) {
    $possible[$vid] = $vocab->name;
  }

  return array(
    'vid' => array(
      '#type' => 'checkboxes',
      '#title' => t('Vocabularies'),
      '#options' => $possible,
      '#default_value' => !empty($options['vid']) ? $options['vid'] : NULL,
    ),
  );
  
}