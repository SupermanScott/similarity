<?php
// $Id: $
/**
 * @file:
 * provides a to filter the possible objects to match a set of node types.
 */
$plugin = array(
  'title' => t('Node type'),
  'callback' => 'similarity_condition_node_type',
  'option_definition' => array(
    'types' => array(
      '#default_value' => array('page', 'story'),
    ),
  ),
  'form_callback' => 'similarity_condition_node_type_form',
  'base' => array('node'),
	'help_topic' => 'conditions-node-type',
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
function similarity_condition_node_type_form($form_state, $options) {
  return array(
    'types' => array(
      '#type' => 'checkboxes',
      '#title' => t('Node Types'),
      '#options' => node_get_types('names'),
      '#default_value' => $options['types'],
    ),
  );
}