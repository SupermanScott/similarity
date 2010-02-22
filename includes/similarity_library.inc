<?php
// $Id: $

/**
 * @file:
 * Provides the library functions for various steps in the similarity process.
 */

/**
 * calculates teh cosine between objects a and b
 *
 * @param $a
 * array of revelancy scores keyed by word
 * ie 'wizard' => 7.33333
 *
 * @param $b
 * array of revelancy scores keyed by word
 * ie 'wizard' => 7.33333
 * 
 * @see get_words
 */
function similarity_cosine($a, $b) {
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
function similarity_taxonomy_tokens($nid) {
  // @TODO: need a way to restrict to vid.
  $tids = db_query("SELECT tid FROM {term_node} WHERE nid = %d", $nid);
  $tokens = array();

  while ($tid_object = db_fetch_object($tids)) {
    // @TODO: calculate tf/idf.
    $tokens[$tid_object->tid] = 1;
  }

  return $tokens;
}