<?php

/**
 * @file
 * Administrative functionality.
 *
 * Form constructor for config settings and submit handler.
 */

/**
 * Menu callback.
 *
 * Main configuration settings page for convertfile.
 *
 * @see convertfile_menu()
 */
function convertfile_config_form($form, &$form_state) {
  $form['installed_on'] = array(
    '#type' => 'fieldset',
    '#title' => 'Widgets assigned to fields',
    '#collapsible' => TRUE, 
    '#collapsed' => TRUE,
  );
  // Display all fields that are using the convertfile_file widget.
  if ($widgets = convertfile_find_widgets('convertfile_file')) {
    $markup = '<div><u>Convert File</u> widget is assigned on:</div><ul>';
    foreach($widgets as $widget) {
      $markup .= "<li>{$widget}</li>";
    }
    $markup .= '</ul>';
  }
  else {
    $markup = '<div>No <u>Convert File</u> widgets are assigned</div>';
  }
  $form['installed_on']['convertfile_file_installed'] = array(
    '#type' => 'markup',
    '#markup' => $markup,
  );
  // Display all fields that are using the convertfile_file widget.
  if ($widgets = convertfile_find_widgets('convertfile_image')) {
    $markup = '<div><u>Convert Image</u> widget is assigned on:</div><ul>';
    foreach($widgets as $widget) {
      $markup .= "<li>{$widget}</li>";
    }
    $markup .= '</ul>';
  }
  else {
    $markup = '<div>No <u>Convert Image</u> widgets are assigned</div>';
  }
  $form['installed_on']['convertfile_image_installed'] = array(
    '#type' => 'markup',
    '#markup' => $markup,
  );

  // Settings.
  $form['convertfile_help'] = array(
    '#type' => 'checkbox',
    '#title' => 'Help text',
    '#description' => 'Enable help text and links which are inserted on various pages.',
    '#default_value' => variable_get('convertfile_help', FALSE),
  );
  $form['convertfile_private_downloads'] = array(
    '#type' => 'checkbox',
    '#title' => t('Private download.'),
    '#description' => t('Enable the download of backup files even if they are in the private directory. There are security implications to this.'),
    '#default_value' => variable_get('convertfile_private_downloads', 0),
  );
  $form['convertfile_inline_convert'] = array(
    '#type' => 'checkbox',
    '#title' => t('Inline convert.'),
    '#description' => t('Enable convert options inline on each managed file field.'),
    '#default_value' => variable_get('convertfile_inline_convert', 0),
  );
  $form['convertfile_inline_revert'] = array(
    '#type' => 'checkbox',
    '#title' => t('Inline revert.'),
    '#description' => t('Enable revert button inline on each managed file field.'),
    '#default_value' => variable_get('convertfile_inline_revert', 0),
  );
  $form['convertfile_to_backup'] = array(
    '#type' => 'checkbox',
    '#title' => t('Convert to backup.'),
    '#description' => t('Normally the current file is converted to the new requested format and a backup of the original is made. Reverse this operation so the current file always stays the same but a new backup is made using the requested conversion format.'),
    '#default_value' => variable_get('convertfile_to_backup', 0),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );
  return $form;
}

/**
 * Submit handler for admin config form.
 */
function convertfile_config_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  variable_set('convertfile_private_downloads', $values['convertfile_private_downloads']);
  variable_set('convertfile_inline_convert', $values['convertfile_inline_convert']);
  variable_set('convertfile_inline_revert', $values['convertfile_inline_revert']);
  if ($values['convertfile_help'] != variable_get('convertfile_help', FALSE)) {
    drupal_set_message(t('Cache may need to be flushed to add or remove some helper text and links.'));
  }
  variable_set('convertfile_help', $values['convertfile_help']);
  variable_set('convertfile_to_backup', $values['convertfile_to_backup']);

  drupal_set_message(t('Settings have been updated.'));
}

/**
 * Menu callback.
 *
 * Manual file conversion page.
 *
 * @see convertfile_menu()
 */
function convertfile_convert_form($form, &$form_state) {
  $provider = isset($form_state['values']['provider']) ? $form_state['values']['provider'] : NULL;
  $format = isset($form_state['values']['format']) ? $form_state['values']['format'] : NULL;

  $form['provider'] = array(
    '#type' => 'select',
    '#title' => t('Provider'),
    '#options' => convertfile_options_provider(),
    '#ajax' => array(
      'callback' => 'convertfile_convert_form_ajax',
      'wrapper' => 'convertfile-ajax',
      'method' => 'replace',
      'effect' => 'fade',
    ),    
  );
  $form['ajax'] = array(
    '#type' => 'markup',
    '#prefix' => '<div id="convertfile-ajax">',
    '#suffix' => '</div>',
  );
  $form['ajax']['format'] = array(
    '#type' => 'select',
    '#title' => t('Format'),
    '#options' => convertfile_options_format($provider),
  );
  $form['ajax']['specific'] = convertfile_options_specific($provider, $format);
  $form['fid_new'] = array(
    '#type' => 'managed_file',
    '#title' => t('Uploaded file.'),
    '#description' => t('Upload a new file to convert.'),
  );
  $form['fid_existing'] = array(
    '#type' => 'textfield',
    '#title' => t('Existing file.'),
    '#description' => t('FID of existing file to convert.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Convert'),
  );
  return $form;
}

/**
 * AJAX callback.
 *
 * Construct provider formats based on provider.
 *
 * @see convertfile_convert_form()
 */
function convertfile_convert_form_ajax($form, &$form_state) {
  return $form['ajax'];
}

/**
 * Submit handler.
 *
 * @see convertfile_convert_form()
 */
function convertfile_convert_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $fid = (isset($values['fid_existing']) && $values['fid_existing']) ? $values['fid_existing'] : NULL;
  $fid = (isset($values['fid_new']) && $values['fid_new']) ? $values['fid_new'] : $fid;
  $provider = isset($values['provider']) ? $values['provider'] : NULL;
  $format = isset($values['format']) ? $values['format'] : NULL;
  if (!$specific = convertfile_options_exposed($provider, $form, &$form_state, $format)) {
    drupal_set_message('Error retrieving provider specific options. Will try to convert anyway.', 'warning');
  }

  if ($fid && $file = file_load($fid)) {
    if (convertfile_convert_file($file, $provider, $format, NULL, $specific)) {
      $url = file_create_url($file->uri);
      drupal_set_message("File has been converted to: <a href='{$url}'>$url</a>" , 'status');
    }
    else {
      drupal_set_message('Unable to convert file. See <a href="/admin/config/convertfile/settings/watchdog">Logs</a> for detailed error.', 'warning');
    }
  }
}

/**
 * Menu callback.
 *
 * Display all active and inactive rules associated with convertfile.
 *
 * @see convertfile_menu()
 * @see rules_admin_reaction_overview()
 */
function convertfile_rules_form($form, &$form_state) {

  // Show all of the active rules.
  $conditions = array(
    'plugin' => 'reaction rule', 
    'active' => TRUE,
    'tags' => array('convertfile'),
  );
  $base_path = 'admin/config/workflow/rules/reaction';
  $options = array('show plugin' => FALSE, 'base path' => $base_path);
  $form['active'] = rules_ui()->overviewTable($conditions, $options);
  $form['active']['#caption'] = t('Active rules');
  $form['active']['#empty'] = t('There are no active rules. <a href="!url">Add new rule</a>.', array('!url' => url('admin/config/workflow/rules/reaction/add')));

  // Show all of the inactive rules.
  $conditions['active'] = FALSE;
  $form['inactive'] = rules_ui()->overviewTable($conditions, $options);
  $form['inactive']['#caption'] = t('Inactive rules');
  $form['inactive']['#empty'] = t('There are no inactive rules.');

  return $form;
}

/**
 * Menu callback.
 *
 * Display all providers.
 */
function convertfile_provider_form() {
  // Display some helpful text if no provider's have been installed yet.
  if (!$info = convertfile_collect_info(TRUE)) {
    $form['hint'] = array(
      '#type' => 'markup',
      '#markup' => '<p>No file conversion providers have been installed. You must ' .
        'install at least one provider to use this module. Convert File comes ' .
        'with some bundled providers.</p>',
    );
    $form['enable'] = array(
      '#title' => 'Enable provider: Convert File',
      '#type' => 'checkbox',
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );
  return $form;
}

/**
 * Submit handler for convertfile_provider_form.
 *
 * @see convertfile_provider_form()
 */
function convertfile_provider_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  if (isset($values['enable']) && $values['enable']) {
    if (module_enable(array('cf_convertfile'), TRUE)) {
      menu_rebuild();
    } else {
      drupal_set_message('Could not enable cf_convertfile.', 'error');
    }
  }

  drupal_set_message(t('Settings have been updated.'));
}

/**
 * Menu callback.
 *
 * Display all debug and error messages from watchdog for our module.
 *
 * @see dblog_overview()
 */
function convertfile_watchdog_form() {
  $rows = array();
  $classes = array(
    WATCHDOG_DEBUG     => 'dblog-debug',
    WATCHDOG_INFO      => 'dblog-info',
    WATCHDOG_NOTICE    => 'dblog-notice',
    WATCHDOG_WARNING   => 'dblog-warning',
    WATCHDOG_ERROR     => 'dblog-error',
    WATCHDOG_CRITICAL  => 'dblog-critical',
    WATCHDOG_ALERT     => 'dblog-alert',
    WATCHDOG_EMERGENCY => 'dblog-emerg',
  );

  // Only include convertfile core and provider messages.
  $modules = array();
  if ($info = convertfile_collect_info()) {
    $modules = array_keys($info);
  }
  $modules[] = 'convertfile';
  $where = '';
  foreach($modules as $module) {
    $where .= (strlen($where)) ? ' OR w.type = ?' : '(w.type = ?';
  }
  $where .= ')';

  $header = array(
    '', // Icon column.
    array('data' => t('Type'), 'field' => 'w.type'),
    array('data' => t('Date'), 'field' => 'w.wid', 'sort' => 'desc'),
    t('Message'),
    array('data' => t('User'), 'field' => 'u.name'),
    array('data' => t('Operations')),
  );

  $query = db_select('watchdog', 'w')->extend('PagerDefault')->extend('TableSort');
  $query->leftJoin('users', 'u', 'w.uid = u.uid');
  $query
    ->fields('w', array('wid', 'uid', 'severity', 'type', 'timestamp', 'message', 'variables', 'link'))
    ->addField('u', 'name');
  $query->where($where, $modules);

  $result = $query
    ->limit(50)
    ->orderByHeader($header)
    ->execute();

  foreach ($result as $dblog) {
    $rows[] = array('data' =>
      array(
        // Cells
        array('class' => 'icon'),
        t($dblog->type),
        format_date($dblog->timestamp, 'short'),
        theme('dblog_message', array('event' => $dblog, 'link' => TRUE)),
        theme('username', array('account' => $dblog)),
        filter_xss($dblog->link),
      ),
      // Attributes for tr
      'class' => array(drupal_html_class('dblog-' . $dblog->type), $classes[$dblog->severity]),
    );
  }

  $build['dblog_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array('id' => 'admin-dblog'),
    '#empty' => t('No log messages available.'),
  );
  $build['dblog_pager'] = array('#theme' => 'pager');

  // Add css.
  drupal_add_css(
    drupal_get_path('module', 'dblog') . '/dblog.css', 
    array('group' => CSS_DEFAULT, 'every_page' => FALSE)
  );

  return $build;
}
