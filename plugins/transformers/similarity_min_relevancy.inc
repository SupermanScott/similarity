<?php
// $Id: $
/**
 * @file:
 * Provides a transformer plugin to remove tokens that are less then its
 * specified relevancy.
 */
$plugin = array(
  'title' => t('Token Relevancy'),
  'callback' => 'similarity_transformer_min_relevancy',
  'form_callback' => 'similarity_transformer_min_relevancy_form',
	'help_topic' => 'transformers-min-relevancy',
  'option_definition' => array(
    'min_relevancy' => array('#default_value' => 0.5),
  ),
);

/**
 * Options form callback.
 */
function similarity_transformer_min_relevancy_form($form_state, $options) {
  return array(
    'min_relevancy' => array(
      '#type' => 'select',
      '#title' => t('Minimum token relevancy score'),
      '#options' => drupal_map_assoc(range(10, 100, 5)),
      '#description' => t('Remove tokens that are less than or equal to this score. This removes irrelevant tokens from the calculation.'),
      '#default_value' => $options['min_relevancy'] * 100,
      '#element_validate' => array('similarity_validate_min_sim')
    ),
  );
}

/**
 * Remove tokens that do not meet our minimum relevancy requirement.
 *
 * @param $tokens
 *   The tokens that were extracted from the database.
 * @param $options
 *   The options for this plugin.
 *
 * @return array
 *   The new tokens.
 */
function similarity_transformer_min_relevancy($tokens, $options) {
  $tokens = array_filter($tokens, Functor::create_functor('similarity_transformer_process_token', $options['min_relevancy']));
  return $tokens;
}

/**
 * Process an individual token.
 */
function similarity_transformer_process_token($token, $min_relevancy) {
  return $token > $min_relevancy;
}

/**
 * A function object class to allow callbacks with arbitrary parameters.
 * Stolen as is from: http://thereisamoduleforthat.com/content/functors-your-single-parameter-callback-blues
 */
class Functor {
  function __construct($callback, $args) {
    $this->callback = $callback;
    $this->args = $args;
  }
  function callback() {
    $args = array_merge(func_get_args(), $this->args);
    return call_user_func_array($this->callback, $args);
  }
  static function create_functor($callback) {
    $args = func_get_args();
    array_shift($args);
    return array(new Functor($callback, $args), 'callback');
  }
  var $callback;
  var $args;
};
