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
 * Find the options for a plugin.
 *
 */
function similarity_find_options($options, $plugin_type, $plugin_id) {
  if (in_array($plugin_type, array('conditions', 'transformers'))) {
    foreach ($options[$plugin_type] as $plugin_info) {
      if ($plugin_info['plugin_id'] == $plugin_id) {
        return $plugin_info['options'];
      }
    }
  }

  return $options[$plugin_type]['options'];
}

/**
 * Edit Similarity form
 */
function similarity_object_edit($form_state, $similarity, $category, $plugin_id) {
  if ($category == 'basics') {
    $form['options'] = $similarity->optionForm($form_state);
  }
  else {
    $plugin_def = ctools_get_plugins('similarity', $category, $plugin_id);
    $form['options'] = $plugin_def['options']['form_id']($form_state, similarity_find_options($similarity->options, $category, $plugin_id));
  }

  $form['options']['#tree'] = TRUE;

  $form['#category'] = $category;
  $form['#similarity'] = $similarity;
  $form['#plugin_id'] = $plugin_id;
  $form['buttons'] = array(
    'save' => array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#attributes' => array(
        'class' => 'ctools-use-ajax',
      ),
      '#id' => 'similarity-save-button',
    ),
  );
  $form['#submit'] = array('similarity_object_edit_submit_' . $category);
  return $form;
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_conditions($form, &$form_state) {
  foreach ($form['#similarity']->options['conditions'] as $key => $plugin_info) {
    if ($plugin_info['plugin_id'] == $form['#plugin_id']) {
      $form['#similarity']->options['conditions'][$key]['options'] = $form_state['values']['options'];
    }
  }
  similarity_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_transformers($form, &$form_state) {
  foreach ($form['#similarity']->options['transformers'] as $key => $plugin_info) {
    if ($plugin_info['plugin_id'] == $form['#plugin_id']) {
      $form['#similarity']->options['transformers'][$key]['options'] = $form_state['values']['options'];
    }
  }
  similarity_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_tokenizers($form, &$form_state) {
  $form['#similarity']->options[$form['#category']]['options'] = $form_state['values']['options'];
  similarity_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_basics($form, &$form_state) {
  $form['#similarity']->options = array_merge($form['#similarity']->options, $form_state['values']['options']);
  similarity_save($form['#similarity']);
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
function similarity_settings_page(Similarity $similarity, $js = FALSE, $action = 'edit', $section = 'basics', $plugin_id = '') {
  drupal_add_css(drupal_get_path('module', 'similarity') . '/theme/similarity.css');
  //ctools_include('ajax');
  //ctools_add_js('ajax-responder');
  //ctools_include('form');
  $config_info = array();
  $basics = array(
    '#title' => t('Basics'),
    '#links' => array(
      'change' => array(
        'title' => t('Edit'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/basics',
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
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/conditions',
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
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/conditions/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
      'remove' => array(
        'title' => t('Remove'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/remove/conditions/' . $plugin_def['plugin_id'],
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

  $tokenizer_plugin = ctools_get_plugins('similarity', 'tokenizers', $similarity->options['tokenizers']['plugin_id']);
  $tokenizer_item = array(
    '#title' => $tokenizer_plugin['title'],
    '#value' => theme('links', array('edit' => array('title' => t('Edit'), 'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/tokenizers/' . $similarity->options['tokenizers']['plugin_id'], 'attributes' => array('class' => 'ctools-use-ajax')),)));

  $tokenizer = array(
    '#title' => t('Tokenizer'),
    '#links' => array(
      'change' => array(
        'title' => t('Change'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/tokenizers',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    $similarity->options['tokenizers']['plugin_id'] => $tokenizer_item,
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
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/transformers',
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
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/transformers/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
      'remove' => array(
        'title' => t('Remove'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/remove/transformers/' . $plugin_def['plugin_id'],
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

  $main_section_function = 'similarity_edit_main_section_' . $action;
  $main_section = "<div class='similarity-main-section'><div class='similarity-main-content'>" . $main_section_function($js, $similarity, $section, $plugin_id) . "</div></div>";

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

/**
 * Return the html for the main section of the edit page for the 'edit' action.
 *
 * @param $js
 *   Boolean on whether or not to return ajax commands.
 * @param Similarity $similarity
 *   The Similarity being edited.
 * @param $section
 *   The section of the similarity object being edited.
 * @param $plugin_id
 *   The id of the plugin being changed can be empty string.
 *
 * @return string
 *   HTML to be render in the main section.
 */
function similarity_edit_main_section_edit($js, Similarity $similarity, $section, $plugin_id) {
  if ($js) {
    $commands = array();
    $form_state = array(
      'ajax' => TRUE,
      'args' => array(
        $similarity,
        $section,
        $plugin_id,
      ),
      're_render' => FALSE,
      'no_redirect' => TRUE,
    );
    $form_render = ctools_build_form('similarity_object_edit', $form_state);
    if (!empty($form_state['ajax']) && empty($form_state['executed'])) {
      $commands[] = ctools_ajax_command_html('div.similarity-main-section > div.similarity-main-content', $form_render);
    }
    else {
      $commands[] = ctools_ajax_command_html('div.similarity-main-section > div.similarity-main-content', 'hi');
    }
    ctools_ajax_render($commands);
  }
  else {
    return drupal_get_form('similarity_object_edit', $similarity, $section, $plugin_id);
  }
}

/**
 * Return the html for the main section of the edit page for the 'add' action.
 *
 * @param $js
 *   Boolean on whether or not to return ajax commands.
 * @param Similarity $similarity
 *   The Similarity being edited.
 * @param $section
 *   The section of the similarity object being edited.
 *
 * @return string
 *   HTML to be render in the main section.
 */
function similarity_edit_main_section_add($section) {
}

/**
 * Return the html for the main section of the edit page for the 'remove' action.
 *
 * @param $js
 *   Boolean on whether or not to return ajax commands.
 * @param Similarity $similarity
 *   The Similarity being edited.
 * @param $section
 *   The section of the similarity object being edited.
 * @param $plugin_id
 *   The id of the plugin being changed can be empty string.
 *
 * @return string
 *   HTML to be render in the main section.
 */
function similarity_edit_main_section_remove($section, $plugin_id) {
}
