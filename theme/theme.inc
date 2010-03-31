<?php
// $Id: $
/**
 * @file:
 * Default theme implementations for similarity module.
 */

/**
 * Default theme implementation for individual similarity objects overview.
 */
function theme_similarity_object_overview($element) {
  $output = '<div class="similarity-object-overview"><span class="similarity-title">' . $element['#title'] . '</span>';
  $output .= '<span class="similarity-admin-links">' . theme('links', $element['#links']) . '</span>';
  $output .= '<div class="similarity-object-description">' . $element['#description'] . '</div>';
  $output .= '</div>';
  return $output;
}

/**
 * Default theme implementation for the edit container.
 */
function theme_similarity_edit_container($container) {
  if (!isset($container['#attributes'])) {
    $container['#attributes'] = array('class' => '');
  }

  if (!isset($container['#attributes']['class'])) {
    $container['#attributes']['class'] = '';
  }

  $container['#attributes']['class'] .= ' similarity-edit-container';
  $container['#attributes']['class'] = trim($container['#attributes']['class']);
  $output = '<div ' . drupal_attributes($container['#attributes']) . '>';
  if ($container['#title']) {
    $output .= '<h4>' . check_plain($container['#title']) . '</h4>';
  }
  if ($container['#links']) {
    $output .= theme('links', $container['#links'], array('class' => 'links similarity-container-links'));
  }

  foreach (element_children($container) as $child) {
    $output .= '<div class="similarity-container-item-wrapper"><span class="similarity-container-item-title">' . $container[$child]['#title'] . '</span><span class="similarity-container-item-value">' . $container[$child]['#value'] . '</span></div>';
  }

  return $output . '</div>';
}