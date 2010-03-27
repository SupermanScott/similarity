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
  drupal_add_css(drupal_get_path('module', 'similarity') . '/theme/similarity.css');
  $objects = similarity_all_objects();

  if (!empty($objects)) {
    $overview = array();
    // Loop through all objects and generate override/configure links.
    foreach ($objects as $name => $sim_object) {
      $overview[$name] = array(
        '#title' => "({$sim_object->type}) {$sim_object->title()}",
        '#description' => $sim_object->description(),
        '#links' => array(
          'edit' => array(
            'href' => 'admin/build/similarity/edit/' . $name,
            'title' => t('edit'),
            'attributes' => array(
              'class' => 'similarity-edit similarity-admin-link',
            ),
          ),
        ),
        '#theme' => array('similarity_object_overview__' . $name, 'similarity_object_overview'),
      );

      if ($sim_object->export_type & EXPORT_IN_DATABASE) {
        $delete_text = 'delete';
        if ($sim_object->export_type & EXPORT_IN_CODE) {
          $delete_text = 'revert';
        }
        $overview[$name]['#links']['delete'] = array(
          'href' => 'admin/build/similarity/delete/' . $name,
          'attributs' => array(
            'class' => 'similarity-delete similarity-admin-link',
          ),
          'title' => t($delete_text),
        );
      }
    }

    return drupal_render($overview);
  }
  else {
    return l(t('There are no similarity objects created yet. Add one now.'), 'admin/build/similarity/add');
  }
}

/**
 * Delete confirmation form.
 */
function similarity_object_delete($form_state, $similarity_object) {
  $button_text = 'Delete';
  if ($similarity_object->export_type & EXPORT_IN_CODE) {
    $button_text = 'Revert';
  }
  $form['#similarity'] = $similarity_object;
  return confirm_form($form, t('Are you sure you want to @text @title', array('@text' => $button_text, '@title' => $similarity_object->title())), 'admin/build/similarity', '', $button_text);
}

/**
 * Delete confirmation submit.
 */
function similarity_object_delete_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/build/similarity';
  db_query("DELETE FROM {similarity_objects} WHERE machine_name = '%s'", $form['#similarity']->machine_name);
}