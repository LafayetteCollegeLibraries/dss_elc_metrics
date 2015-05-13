<?php

/**
 * @author griffinj@lafayette.edu
 * ELC Metrics Reporting functionality
 *
 */

class Report {

  public static $date_fields = array('field_loan_duration');
  public static $taxonomy_fields = array('field_item_subject');

  }

function dss_elc_metrics_report($bundles = array('loan','item'),
				$field_u = 'field_loan_duration',
				$field_v = 'field_item_subject',
				$csv_file_path = NULL
				) {

  module_load_include('inc', 'dss_elc_metrics',  'includes/apachesolr');

  $entity_reference_field = 'field_bib_rel_object';

  $query = new EntityFieldQuery();
  $entities = $query->entityCondition('entity_type', 'node', '=')
    ->entityCondition('bundle', 'loan')
    ->range(0, 2000)
    ->execute();
  $entities_u = node_load_multiple(array_keys($entities['node']));

  $field_u_bin_labels = array('1811 - 1819', '1820 - 1829', '1830 - 1839');
  $field_u_bins = array('1811 - 1819 (Total)' => 0,
			'1820 - 1829 (Total)' => 0,
			'1830 - 1839 (Total)' => 0,
			'After 1839 (Total)' => 0);

  $field_u_grouped_bins = array();
  $field_v_groups = array();
  $field_v_groups_filter = array('Fiction', 'Biography', 'History', 'Travel', 'Belles Letters');

  $entity_reference_field_values = array();

  $date_fields = array('field_loan_duration');
  $taxonomy_fields = array('field_item_subject');

  foreach($entities_u as $entity_u) {

    // For retrieving the related entities
    $values = $entity_u->{$entity_reference_field}[$entity_u->language];
    if( empty($values) or $values[0]['target_id'] == '0' ) {

      continue;
    }

    $entity_id = $values[0]['target_id'];

    // Retrieve the decade
    $bin_values = $entity_u->{$field_u}[$entity_u->language];
    $bin_value = $bin_values[0]['value'];

    if(in_array($field_u, $date_fields)) {

      if(preg_match('/^\d+$/', $bin_value)) {

	$date_value = DateTime::createFromFormat('U', $bin_value);
      } else {

	$date_value = new DateTime($bin_value);
      }

      $decade = intval($date_value->format('Y'));

      // Construct the bins for each decade
      $bin = intval(($decade - 1811) / 9);

      if(!array_key_exists($bin, $field_u_bin_labels)) {

	$bin_label = 'After 1839';
      } else {

	$bin_label = $field_u_bin_labels[$bin];
      }
    }

    $field_u_bins[$bin_label . " (Total)"] += 1;

    // Initialize the bins for field U, grouped by field V
    if( !array_key_exists($bin_label, $field_v_groups) ) {

      $field_v_groups[$bin_label] = array();
    }

    // Retrieve the related Item
    $query = new EntityFieldQuery();
    $entities = $query->entityCondition('entity_type', 'node', '=')
      ->fieldCondition('field_item_embodies', 'target_id', $entity_id)
      ->range(0,1)
      ->execute();
    $entity_v = node_load(array_shift(array_keys($entities['node'])));

    $values = $entity_v->{$field_v}[$entity_v->language];
    if( empty($values) ) {

      continue;
    }

    if(in_array($field_v, $taxonomy_fields) and !empty($values) ) {

      $tid = $values[0]['tid'];
      $term = taxonomy_term_load($tid);

      // Filter the terms
      // (Easier than a map->reduce)
      $is_filtered = FALSE;
      foreach($field_v_groups_filter as $filter_term) {

	$is_filtered = $is_filtered || (bool) preg_match("/^$filter_term/", $term->name);
      }

      if($is_filtered) {

	$field_v_groups[$bin_label][$term->name] += 1;
	$field_u_grouped_bins[$bin_label . " ({$term->name})"] += 1;
      }
    }
  }

  $field_u_bins = array_merge($field_u_bins, $field_u_grouped_bins);

  if( is_null($csv_file_path) ) {

    print implode(',', array_keys($field_u_bins)) . "\n";
    print implode(',', array_values($field_u_bins));
    
  } else {

    // Write to the CSV-formatted file
    $fp = fopen($csv_file_path, 'w');

    fputcsv($fp, array_keys($field_u_bins) );
    fputcsv($fp, array_values($field_u_bins) );

    fclose($fp);
  }
}
