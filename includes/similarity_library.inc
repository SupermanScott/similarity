<?php
// $Id: $

/**
 * @file:
 * Provides the library functions for various steps in the similarity process.
 */

/**
 * calculates the cosine between objects a and b
 *
 * @param $a
 * array of relevancy scores keyed by word i.e 'wizard' => 7.33333.
 *
 * @param $b
 *   array of relevancy scores keyed by word i.e 'wizard' => 7.33333.
 * 
 * @param $options
 *  $options from our settings form.
 *
 * @return float
 *  The similarity score in between 0 and 1
 */
function similarity_cosine($a, $b, $options) {
  $dot_product = 0;
  $a_total = 0;
  $b_total = 0;
  
  $total_words = array_unique(array_merge(array_keys($a), array_keys($b)));

  // only care about the key $word
  foreach($total_words as $word) {
    if (isset($a[$word]) && isset($b[$word])) {
      $dot_product += $a[$word] * $b[$word];
    }
    
    if (isset($a[$word])) {
      $a_total += pow($a[$word], 2);
    }
    
    if (isset($b[$word])) {
      $b_total += pow($b[$word], 2);
    }
  }
  
  // if nothing worked
  if ($a_total == 0 || $b_total == 0 ) {
    return 0;
  }

  return ($dot_product / sqrt($a_total * $b_total));
}

/**
 * Taxonomy tokenizer.
 *
 * @param $nid
 *  The node id for these tokens.
 *
 * @return array
 *  a tid => tf/idf array for all terms assigned to the node.
 */
function similarity_taxonomy_tokens($nid, $options) {
  static $total_nodes = NULL, $idf = array();
  // @TODO: investigate creating a query based on the Similarity->conditions.
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
 * Node Type condition
 *
 * @param array $query
 *  Structure array representing the query.
 *
 * @param $options
 *  An array of option values.
 */
function similarity_condition_node_type($query, $options) {
  if (!empty($options['types'])) {
    $query['filters']['type'] = 'node.type IN(' . db_placeholders($options['types'], 'varchar') . ')';
    $query['arguments'] += $options['types'];
  }
  return $query;
}