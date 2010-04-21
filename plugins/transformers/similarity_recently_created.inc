<?php
// $Id: $
/**
 * @file:
 * Provides a transformer plugin to make more recently created nodes tokens more
 * relevant.
 */
$plugin = array(
  'title' => t('Recently Created Boost'),
  'callback' => 'similarity_transformer_created_boost',
	'help_topic' => 'transformers-created-boost',
  'base' => 'node',
);

/**
 * Boost all tokens by the the nodes created time.
 *
 * @param int $id
 *   The object id being tokenized.
 * @param $tokens
 *   The tokens that were extracted from the database.
 * @param $options
 *   The options for this plugin.
 *
 * @return array
 *   The new tokens.
 */
function similarity_transformer_created_boost($id, $tokens, $options) {
  $created = db_result(db_query("SELECT created FROM {node} WHERE nid = %d", $id));
  $new_tokens = array();
  foreach ($tokens as $word => $relevancy) {
    $new_tokens[$word] = $relevancy + $created;
  }

  return $new_tokens;
}
