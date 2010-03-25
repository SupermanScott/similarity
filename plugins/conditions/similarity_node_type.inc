<?php
// $Id: $
/**
 * @file:
 * provides a to filter the possible objects to match a set of node types.
 */
$plugin = array(
  'title' => t('Node type'),
  'callback' => 'similarity_condition_node_type',
  'options' => array(
    'form_id' => 'similarity_condition_node_type_form',
    'definition' => array(),
  ),
  'base' => array('node'),
);

/**
 * Node Type condition
 *
 * @param array $query
 *   Structure array representing the query.
 * @param $options
 *   An array of option values.
 */
function similarity_condition_node_type($query, $options) {
  if (!empty($options['types'])) {
    $query['filters']['type'] = 'node.type IN(' . db_placeholders($options['types'], 'varchar') . ')';
    $query['arguments'] += $options['types'];
  }
  return $query;
}

/**
 * Options form for the node type plugin.
 */
function similarity_condition_node_type_form($form_state) {
}