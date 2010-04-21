<?php
// $Id: $
/**
 * @file:
 * provides a tokenizer plugin that uses the search index as its basis.
 */
$plugin = array(
  'title' => t('Search Index'),
  'callback' => 'similarity_tokenizer_search_index',
  'reindex_candidates_callback' => 'similarity_tokenizer_search_reindex',
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

/**
 * Reindex callback. Returns all the ids that should be reindex.
 *
 * @param $id
 *  The id that recently changed.
 * @param $options
 *  The plugin options.
 */
function similarity_tokenizer_search_reindex($id, $options) {
  $id_words = db_query("SELECT word FROM {search_index} WHERE sid = %d AND type = 'node'", $id);
  $words = array();
  while ($word_object = db_fetch_object($id_words)) {
    $words[] = $word_object->word;
  }

  $ids = array();
  if (!empty($words)) {
    $sql = "SELECT DISTINCT(sid) FROM {search_index} WHERE word IN (" . db_placeholders($words, 'varchar') . ")";
    $id_result = db_query($sql, $words);
    while ($id_result = db_fetch_object($id_result)) {
      $ids[] = $id_result->sid;
    }
  }

  return $ids;
}