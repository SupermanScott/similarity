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

/**
 * Edit Similarity form
 */
function similarity_object_edit($form_state, $similarity) {
  return $similarity->optionForm($form_state);
}

/**
 * Similarity settings page.
 *
 * @param Similarity $similarity
 *   The Similarity that is being changed.
 *
 * @return string
 *   HTML string for the output.
 */
function similarity_settings_page(Similarity $similarity) {
  drupal_add_css(drupal_get_path('module', 'similarity') . '/theme/similarity.css');
  $config_info = array();
  $basics = array(
    '#title' => t('Basics'),
    '#links' => array(
      'change' => array(
        'title' => t('Edit'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/basics',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    'title' => array(
      '#type' => 'item',
      '#title' => t('Human readable title'),
      '#value' => $similarity->title(),
    ),
    'min_sim' => array(
      '#type' => 'item',
      '#title' => t('Minimum similarity score'),
      '#value' => $similarity->options['min_sim'],
    ),
    '#theme' => array('similarity_edit_container__basics', 'similarity_edit_container'),
    '#attributes' => array(
      'class' => 'similarity-container-basics',
    ),
  );

  $conditions = array(
    '#title' => t('Conditions'),
    '#links' => array(
      'add' => array(
        'title' => t('Add'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/condition',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    '#theme' => array('similarity_edit_container__conditions', 'similarity_edit_container'),
    '#attributes' => array(
      'class' => 'similarity-container-conditions',
    ),
  );

  foreach ($similarity->options['conditions'] as $plugin_def) {
    $plugin = ctools_get_plugins('similarity', 'conditions', $plugin_def['plugin_id']);
    $edit_remove = array(
      'edit' => array(
        'title' => t('Edit'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/condition/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
      'remove' => array(
        'title' => t('Remove'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/remove/condition/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    );

    $conditions[$plugin_def['plugin_id']] = array(
      '#type' => 'item',
      '#title' => $plugin['title'],
      '#value' => theme('links', $edit_remove, array('class' => 'links similarity-plugin-links')),
    );
  }

  $tokenizer_plugin = ctools_get_plugins('similarity', 'tokenizers', $similarity->options['tokenizer']['plugin_id']);
  $tokenizer_item = array(
    '#title' => $tokenizer_plugin['title'],
    '#value' => theme('links', array('edit' => array('title' => t('Edit'), 'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/' . $similarity->options['tokenizer']['plugin_id']),
                                    )));

  $tokenizer = array(
    '#title' => t('Tokenizer'),
    '#links' => array(
      'change' => array(
        'title' => t('Change'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/change/tokenzier',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    $similarity->options['tokenizer']['plugin_id'] => $tokenizer_item,
    '#theme' => array('similarity_edit_container__tokenizer', 'similarity_edit_container'),
    '#attributes' => array(
      'class' => 'similarity-container-tokenizer',
    ),
  );

  $transformers = array(
    '#title' => t('Transformers'),
    '#links' => array(
      'add' => array(
        'title' => t('Add'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/transformer',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    '#theme' => array('similarity_edit_container__transformers', 'similarity_edit_container'),
    '#attributes' => array(
      'class' => 'similarity-container-transformers',
    ),
  );

  foreach ($similarity->options['transformers'] as $plugin_def) {
    $plugin = ctools_get_plugins('similarity', 'transformers', $plugin_def['plugin_id']);
    $edit_remove = array(
      'edit' => array(
        'title' => t('Edit'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/transformer/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
      'remove' => array(
        'title' => t('Remove'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/remove/transformer/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    );

    $transformers[$plugin_def['plugin_id']] = array(
      '#type' => 'item',
      '#title' => $plugin['title'],
      '#value' => theme('links', $edit_remove, array('class' => 'links similarity-plugin-links')),
    );
  }

  $main_section = "<div class='similarity-main-section'><div class='similarity-main-content'>text</div></div>";

  return '<div id="similarity-settings-content--' . $similarity->machine_name . '">' .
    '<div class="similarity-left-column">' .
    drupal_render($basics) .
    drupal_render($conditions) .
    drupal_render($tokenizer) .
    drupal_render($transformers) .
    '</div>' .
    $main_section .
    '</div>';
}
