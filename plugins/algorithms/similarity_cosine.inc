<?php
// $Id: $
/**
 * @file:
 * Algorithm plugin to compute the cosine similarity.
 */
$plugin = array(
  'title' => t('Cosine Similarity'),
  'callback' => 'similarity_algorithm_cosine',
  'options' => array(
    'definition' => array(),
  ),
  // Base properties are ignored for algorithms.
);

/**
 * calculates the cosine between objects a and b
 *
 * @param $a
 *   array of relevancy scores keyed by word i.e 'wizard' => 7.33333.
 * @param $b
 *   array of relevancy scores keyed by word i.e 'wizard' => 7.33333.
 * @param $options
 *  $options from our settings form.
 * @return float
 *  The similarity score in between 0 and 1
 */
function similarity_algorithm_cosine($a, $b, $options) {
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