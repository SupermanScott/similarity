<?php
// $Id: similarity.views.inc,v 1.1.2.2 2009/03/13 21:14:23 scottreynolds Exp $

/**
 * @file similarity.views.inc
 * lovingly crafted by Scott Reynolds
 *
 * Exposes all similarity objects to views
 */

/**
 * Implementation of hook_views_data().
 */
function similarity_views_data() {
  $data = array();
  $all = similarity_all_objects(TRUE);
  if (!empty($all)) {
    foreach($all as $sim_obj) {
      $data = array_merge($data, $sim_obj->viewsData());
    }
  }
  return $data;
}