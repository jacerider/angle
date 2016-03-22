<?php
/**
 * @file
 * Contains functions only needed for drush integration.
 */

/**
 * Implementation of hook_drush_command().
 */
function angle_drush_command() {
  $items = array();

  $items['angle-generate'] = array(
    'description' => 'Create a Angle sub-theme',
    'aliases' => array('angle'),
    'arguments' => array(
      'name'         => 'Your sub-theme name.',
    ),
    'options' => array(
      'machine_name' => 'A machine-readable name for your theme, optional. Machine name must be lowercase only, with underscores instead of spaces.',
      'description'  => 'Your sub-theme description.',
    ),
    'examples' => array(
      'drush angle "custom theme name"' => 'Create a sub-theme with the default options.',
      'drush angle "foo bar" "foo_bar" --description="My custom Angle theme."' => 'Create a sub-theme with additional options.',
    ),
  );

  return $items;
}

/**
 * Create a Angle sub-theme.
 */
function drush_angle_generate($name, $machine_name = NULL) {
  $description = drush_get_option('description');
  // $machine_name = drush_get_option('machine_name');

  if (empty($name)) {
    drush_set_error(dt("Please provide a name for the sub-theme.\nUSAGE:\tdrush angle [name] [machine_name !OPTIONAL] --description=[description !OPTIONAL]\n"));
    return;
  }

  //Filter everything but letters, numbers, underscores, and hyphens
  $machine_name = !empty($machine_name) ? preg_replace('/[^a-z0-9_-]+/', '', strtolower($machine_name)) : preg_replace('/[^a-z0-9_-]+/', '', strtolower($name));
  // Eliminate hyphens
  $machine_name = str_replace('-', '_', $machine_name);

  // Find theme paths.
  $root_path = drush_get_context('DRUSH_DRUPAL_ROOT');
  $angle_path = $root_path . '/' . drupal_get_path('theme', 'angle');
  $app_path = $root_path . '/app';
  $subtheme_path = dirname($angle_path) . '/' . $machine_name;
  $relative_path = str_replace('angle', $machine_name, drupal_get_path('theme', 'angle'));

  // Download Angle seed
  $seed_source = 'https://github.com/jacerider/angle-seed/archive/master.zip';
  $seed_dest = "$angle_path/seed.zip";
  $seed_folder = "$angle_path/angle-seed-master";
  if (!drush_download_file($seed_source, $seed_dest)) {
    drupal_log(dt('Angle Seed could not be downloaded.'), LogLevel::ERROR);
    return;
  }
  drush_register_file_for_deletion($seed_dest);
  if (!drush_file_is_tarball($seed_dest)) {
    drupal_log(dt('Angle Seed was not a tarball.'), LogLevel::ERROR);
    return;
  }
  drush_tarball_extract($seed_dest);
  if (!file_exists($seed_folder)) {
    drupal_log(dt('Angle Seed was not extracted properly.'), LogLevel::ERROR);
    return;
  }
  drush_register_file_for_deletion($seed_folder);
  $s = drush_copy_dir($seed_folder, $app_path);
  if (empty($s)) {
    drupal_log(dt('Angle Seed could not be coppied to destination.'), LogLevel::ERROR);
  }

  // Merge in starter files.
  $s = drush_copy_dir("$angle_path/STARTER/", $subtheme_path);
  if (empty($s)) {
    drupal_log(dt('Theme could not be cloned.'), LogLevel::ERROR);
  }

  // Rename files and fill in the theme machine name
  drush_op('rename', "$subtheme_path/STARTER.info.yml.txt", "$subtheme_path/$machine_name.info.yml");
  drush_op('rename', "$subtheme_path/STARTER.theme", "$subtheme_path/$machine_name.theme");
  drush_op('rename', "$subtheme_path/templates/html.html.twig.txt", "$subtheme_path/templates/html.html.twig");

  // Update the .info file
  drush_op('angle_file_str_replace', "$subtheme_path/$machine_name.info.yml", 'Angle Starter', "$name");

  // Update the .md file
  drush_op('angle_file_str_replace', "$subtheme_path/README.md", 'Angle Starter', "$name");

  // Change the description of the theme
  if (!empty($description)) {
    drush_op('angle_file_str_replace', "$subtheme_path/$machine_name.info.yml", 'Drupal + Angular', "$description");
  }

  // Replace STARTER in info.yml
  drush_op('angle_file_str_replace', "$subtheme_path/$machine_name.info.yml", 'STARTER', "$machine_name");

  // Rename functions in .theme
  drush_op('angle_file_str_replace', "$subtheme_path/$machine_name.theme", 'STARTER', "$machine_name");

  // Replace NAME and DESCRIPTION in package.json
  drush_op('angle_file_str_replace', "$app_path/package.json", 'angle-seed', "$machine_name");
  if (!empty($description)) {
    drush_op('angle_file_str_replace', "$app_path/package.json", 'Seed for Angle Drupal theme', "$description");
  }

  // Notify user of the newly created theme.
  drush_print(dt("\n!name sub-theme was created in !path. \n",
    array(
      '!name' => $name,
      '!path' => $subtheme_path,
    )
  ));
}

/**
 * Internal helper: Replace strings in a file.
 */
function angle_file_str_replace($file_path, $find, $replace) {
  $file_contents = file_get_contents($file_path);
  $file_contents = str_replace($find, $replace, $file_contents);
  file_put_contents($file_path, $file_contents);
}

/**
 * Implements hook_drush_help().
 */
function angle_drush_help($section) {
  switch ($section) {
    case 'drush:angle-sub-theme':
      return dt("Create a sub-theme based on Angle.");
  }
}
