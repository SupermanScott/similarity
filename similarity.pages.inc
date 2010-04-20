<?php
// $Id:$
/**
 * @file:
 * Page callbacks for similarity module.
 */

/**
 * Export a Similarity object.
 */
function similarity_export_page(Similarity $similarity) {
  $code = similarity_export($similarity->exportObject());
  return drupal_get_form('ctools_export_form', $code, check_plain($similarity->description));
}

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
          'bulk_import' => array(
            'href' => 'admin/build/similarity/batch/' . $name,
            'title' => t('calculate'),
            'attributes' => array(
              'class' => 'similarity-bulk-import similarity-admin-link',
            ),
            'query' => array(
              'token' => drupal_get_token($name),
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
 * Commit form.
 *
 */
function similarity_commit_form($form_state, $similarity) {
  return array(
    '#similarity' => $similarity,
    'buttons' => array(
      'commit' => array(
        '#type' => 'submit',
        '#value' => t('Commit Changes'),
      ),
      'cancel' => array(
        '#type' => 'submit',
        '#value' => t('Cancel'),
      ),
    ),
  );
}

/**
 * Submit function to handle the similarity commit form.
 */
function similarity_commit_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Commit Changes')) {
    similarity_save($form['#similarity']);
  }
  similarity_cache_remove($form['#similarity']);
  drupal_set_message(t('All changes to @title have been committed', array('@title' => $form['#similarity']->title())));
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
  similarity_delete($form['#similarity']);
}

/**
 * Find the options for a plugin.
 */
function similarity_find_options($options, $plugin_type, $plugin_id) {
  if (in_array($plugin_type, array('conditions', 'transformers', 'tokenizers'))) {
    foreach ($options[$plugin_type] as $plugin_info) {
      if ($plugin_info['plugin_id'] == $plugin_id) {
        return $plugin_info['options'];
      }
    }
  }

  return $options[$plugin_type]['options'];
}

/**
 * Add a plugin form
 */
function similarity_add_plugin($form_state, $similarity, $category) {
  $plugins = ctools_get_plugins('similarity', $category);
  foreach ($plugins as $plugin_id => $plugin) {
    $options[$plugin_id] = $plugin['title'];
  }
  return array(
    $category => array(
      '#title' => t(drupal_ucfirst($category)),
      '#type' => 'select',
      '#options' => $options,
    ),
    'add' => array(
      '#type' => 'submit',
      '#value' => t('Add'),
    ),
    '#similarity' => $similarity,
    '#submit' => array('similarity_add_plugin_' . $category . '_submit'),
  );
}

/**
 * Form submit callback.
 */
function similarity_add_plugin_transformers_submit($form, &$form_state) {
  $plugin = ctools_get_plugins('similarity', 'transformers', $form_state['values']['transformers']);
  $form['#similarity']->options['transformers'][] = array(
    'plugin_id' => $form_state['values']['transformers'],
    'options' => similarity_compress_option_definition($plugin['option_definition']),
  );


  similarity_cache_save($form['#similarity']);
  $form_state['redirect'] = 'admin/build/similarity/edit/' . $form['#similarity']->machine_name . '/nojs/edit/transformers/' .  $form_state['values']['transformers'];
}

/**
 * Form submit callback.
 */
function similarity_add_plugin_conditions_submit($form, &$form_state) {
  $plugin = ctools_get_plugins('similarity', 'conditions', $form_state['values']['conditions']);
  $form['#similarity']->options['conditions'][] = array(
    'plugin_id' => $form_state['values']['conditions'],
    'options' => similarity_compress_option_definition($plugin['option_definition']),
  );


  similarity_cache_save($form['#similarity']);
  $form_state['redirect'] = 'admin/build/similarity/edit/' . $form['#similarity']->machine_name . '/nojs/edit/conditions/' .  $form_state['values']['conditions'];
}

/**
 * Form submit callback.
 */
function similarity_add_plugin_tokenizers_submit($form, &$form_state) {
  $plugin = ctools_get_plugins('similarity', 'tokenizers', $form_state['values']['tokenizers']);
  $form['#similarity']->options['tokenizers'][] = array(
    'plugin_id' => $form_state['values']['tokenizers'],
    'options' => similarity_compress_option_definition($plugin['option_definition']),
  );


  similarity_cache_save($form['#similarity']);
  $form_state['redirect'] = 'admin/build/similarity/edit/' . $form['#similarity']->machine_name . '/nojs/edit/tokenizers/' .  $form_state['values']['tokenizers'];
}

/**
 * Add a similarity form.
 */
function similarity_add_object_form($form_state) {
  return array(
    'machine_name' => array(
      '#title' => t('Machine Name'),
      '#type' => 'textfield',
      '#required' => TRUE,
      '#description' => t('This is the unique name of the similarity objects. It must contain only alphanumeric characters and underscores; it is used to identify the similarity object internally.'),
      '#element_validate' => array(/* Element validate here */),
    ),
    'title' => array(
      '#title' => t('Name'),
      '#type' => 'textfield',
      '#required' => TRUE,
      '#description' => t('This is the name of the similarity object and is shown only on administration pages.'),
    ),
    'description' => array(
      '#title' => t('Description'),
      '#type' => 'textarea',
      '#description' => t('A description of what this similarity object does.'),
    ),
    'create' => array(
      '#type' => 'submit',
      '#value' => t('Create'),
    ),
  );
}

/**
 * Form submit for adding a similarity.
 */
function similarity_add_object_form_submit($form, &$form_state) {
  $basic_obj = new stdClass();
  $basic_obj->machine_name = $form_state['values']['machine_name'];
  $basic_obj->options = array();
  $basic_obj->options['title'] = $form_state['values']['title'];
  $basic_obj->options['description'] = $form_state['values']['description'];

  $sim = similarity_create($basic_obj);
  similarity_cache_save($sim);

  $form_state['redirect'] = 'admin/build/similarity/edit/' . $sim->machine_name;
}

/**
 * Edit Similarity form
 */
function similarity_object_edit($form_state, $similarity, $category, $plugin_id) {
  if ($category == 'basics') {
    $form['options'] = $similarity->optionForm($form_state);
  }
  else {
    $function = ctools_plugin_load_function('similarity', $category, $plugin_id, 'form_callback');
    if ($function) {
      $form['options'] = $function($form_state, similarity_find_options($similarity->options, $category, $plugin_id));
    }
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
  if (isset($form_state['values']['options'])) {
    foreach ($form['#similarity']->options['conditions'] as $key => $plugin_info) {
      if ($plugin_info['plugin_id'] == $form['#plugin_id']) {
        $form['#similarity']->options['conditions'][$key]['options'] = $form_state['values']['options'];
      }
    }
  }
  similarity_cache_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_transformers($form, &$form_state) {
  if (isset($form_state['values']['options'])) {
    foreach ($form['#similarity']->options['transformers'] as $key => $plugin_info) {
      if ($plugin_info['plugin_id'] == $form['#plugin_id']) {
        $form['#similarity']->options['transformers'][$key]['options'] = $form_state['values']['options'];
      }
    }
  }
  similarity_cache_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_tokenizers($form, &$form_state) {
  if (isset($form_state['values']['options'])) {
    foreach ($form['#similarity']->options['tokenizers'] as $key => $plugin_info) {
      if ($plugin_info['plugin_id'] == $form['#plugin_id']) {
        $form['#similarity']->options['tokenizers'][$key]['options'] = $form_state['values']['options'];
      }
    }
  }
  similarity_cache_save($form['#similarity']);
}

/**
 * FAPI Submit callback.
 */
function similarity_object_edit_submit_basics($form, &$form_state) {
  if (isset($form_state['values']['options'])) {
    $form['#similarity']->options = array_merge($form['#similarity']->options, $form_state['values']['options']);
  }
  similarity_cache_save($form['#similarity']);
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
  // @TODO: handle $similarity->lock.
  drupal_add_css(drupal_get_path('module', 'similarity') . '/theme/similarity.css');
  //ctools_include('ajax');
  //ctools_add_js('ajax-responder');
  //ctools_include('form');
  $config_info = array();
  $basics = array(
    '#title' => t('Basics'),
    '#help' => theme('advanced_help_topic', 'similarity', 'about'),
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
    '#help' => theme('advanced_help_topic', 'similarity', 'conditions'),
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

    if (!empty($plugin['help_topic'])) {
      $conditions[$plugin_def['plugin_id']]['#help'] = theme('advanced_help_topic', $plugin['module'], $plugin['help_topic']);
    }
  }

  $tokenizers = array(
    '#title' => t('Tokenizers'),
    '#help' => theme('advanced_help_topic', 'similarity', 'tokenizers'),
    '#links' => array(
      'add' => array(
        'title' => t('Add'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/add/tokenizers',
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    ),
    '#theme' => array('similarity_edit_container__tokenizers', 'similarity_edit_container'),
    '#attributes' => array(
      'class' => 'similarity-container-tokenizers',
    ),
  );

  foreach ($similarity->options['tokenizers'] as $plugin_def) {
    $plugin = ctools_get_plugins('similarity', 'tokenizers', $plugin_def['plugin_id']);
    $edit_remove = array(
      'edit' => array(
        'title' => t('Edit'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/edit/tokenizers/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
      'remove' => array(
        'title' => t('Remove'),
        'href' => 'admin/build/similarity/edit/' . $similarity->machine_name . '/nojs/remove/tokenizers/' . $plugin_def['plugin_id'],
        'attributes' => array(
          'class' => 'ctools-use-ajax',
        ),
      ),
    );
    $tokenizers[$plugin_def['plugin_id']] = array(
      '#title' => $plugin['title'],
      '#value' => theme('links', $edit_remove, array('class' => 'links similarity-plugin-links')),
    );
    if (!empty($plugin['help_topic'])) {
      $tokenizers[$plugin_def['plugin_id']]['#help'] = theme('advanced_help_topic', $plugin['module'], $plugin['help_topic']);
    }
  }

  $transformers = array(
    '#title' => t('Transformers'),
    '#help' => theme('advanced_help_topic', 'similarity', 'transformers'),
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

    if (!empty($plugin['help_topic'])) {
      $transformers[$plugin_def['plugin_id']]['#help'] = theme('advanced_help_topic', $plugin['module'], $plugin['help_topic']);
    }
  }

  $main_section_function = 'similarity_edit_main_section_' . $action;
  $main_section = "<div class='similarity-main-section'><div class='similarity-main-content'>" . $main_section_function($js, $similarity, $section, $plugin_id) . "</div></div>";

  return '<div id="similarity-settings-content--' . $similarity->machine_name . '">' .
    '<div class="similarity-left-column">' .
    drupal_render($basics) .
    drupal_render($conditions) .
    drupal_render($tokenizers) .
    drupal_render($transformers) .
    '</div>' .
    $main_section .
    '</div>' .
    drupal_get_form('similarity_commit_form', $similarity);
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
function similarity_edit_main_section_add($js, $similarity, $section) {
  if ($js) {
    // CTools commands here.
  }
  else {
    return drupal_get_form('similarity_add_plugin', $similarity, $section);
  }
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
function similarity_edit_main_section_remove($js, Similarity $similarity, $section, $plugin_id) {
  if ($js) {
    // CTools commands here.
  }
  else {
    if (in_array($section, array('conditions', 'transformers', 'tokenizers'))) {
      foreach ($similarity->options[$section] as $key => $plugin_info) {
        if ($plugin_info['plugin_id'] == $plugin_id) {
          unset($similarity->options[$section][$key]);
          break;
        }
      }
    }
    similarity_cache_save($similarity);
    drupal_goto('admin/build/similarity/edit/' . $similarity->machine_name);
  }
}

/**
 * Form for importing a Similarity
 */
function similarity_import_form($form_state) {
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Similarity Name'),
    '#description' => t('Enter the name of the new Similarity. This is optional and is not necessary if you do not wish to rename the object.'),
  );

  $form['object'] = array(
    '#type' => 'textarea',
    '#title' => t('Paste variant code here'),
    '#rows' => 30,
    '#required' => TRUE,
  );

  $form['create'] = array(
    '#type' => 'submit',
    '#value' => t('Create'),
  );

  return $form;
}

/**
 * Make sure that an import actually provides a handler.
 */
function similarity_import_form_validate($form, &$form_state) {
  // First, run the PHP and turn the input code into an object.
  ob_start();
  eval($form_state['values']['object']);
  ob_end_clean();

  // The object should appear as $similarity_object. This was the "identifier" set in the export section of the schema.
  if (empty($similarity_object)) {
    $errors = ob_get_contents();
    if (empty($errors)) {
      $errors = t('No similarity_object found.');
    }

    form_error($form['object'], t('Unable to get a similarity_object from the import. Errors reported: @errors', array('@errors' => $errors)));
  }

  $form_state['similarity'] = similarity_create($similarity_object);
}

/**
 * Save the imported object.
 */
function similarity_import_form_submit($form, &$form_state) {
  $similarity = $form_state['similarity'];

  if (!empty($form_state['values']['name'])) {
    $similarity->machine_name = $form_state['values']['name'];
  }
  similarity_cache_save($similarity);
  $form_state['redirect'] = 'admin/build/similarity/edit/' . $similarity->machine_name;
}

/**
 * Batch page callback to calculate the similarities for the supplied node.
 *
 * @param Similarity $similarity
 *  The Similarity object to batch calculate.
 */
function similarity_batch_import(Similarity $similarity) {
  similarity_reset_similarity($similarity);
  $batch = array(
    'title' => t('Calculating'),
    'operations' => array(),
  );

  foreach ($similarity->findCandidates() as $id) {
    $batch['operations'][] = array('similarity_calculate_work', array(array('id' => $id, 'machine_name' => $similarity->machine_name)));
  }
  
  batch_set($batch);
  batch_process('admin/build/similarity');
}