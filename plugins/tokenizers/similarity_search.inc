<?php
// $Id: $
/**
 * @file:
 * provides a tokenizer plugin that uses the search index as its basis.
 */
$plugin = array(
  'title' => t('Search Index'),
  'callback' => 'similarity_tokenizer_search_index',
  'option_definition' => array(),
	'help_topic' => 'tokenizer-search',
  'base' => array('node'),
);

/**
 * Search Index tokenizer callback.
 *
 * @param $sid
 *   The unique id for this object.
 * @param $options
 *   The options for this tokenization.
 * @param $base
 *   The base table for this tokenization.
 *
 * @return array
 *   a word => relevancy array for all words in the search index for this node.
 */
function similarity_tokenizer_search_index($sid, $options, $base = 'node') {
  $word_result_set = db_query("SELECT word, score FROM {search_index} WHERE sid = %d AND type = '%s'", $sid, $base);

  $words = array();
  while ($word_row = db_fetch_object($word_result_set)) {
    $words[$word_row->word] = $word_row->score;
  }

  return $words;
}