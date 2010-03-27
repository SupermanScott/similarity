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