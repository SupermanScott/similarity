<?php
// $Id:$
/**
 * @file:
 * Page callbacks for similarity module.
 */

/**
 * Page callback for the overview page.
 */
function similarity_objects_overview() {
  $objects = similarity_all_objects();

  if (!empty($objects)) {
    // Loop through all objects and generate override/configure links.
  }
  else {
    return l(t('There are no similarity objects created yet. Add one now.'), 'admin/build/similarity/add');
  }
}