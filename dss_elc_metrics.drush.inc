<?php

  /**
   * Drush hook implementations and functionality
   * @author griffinj@lafayette.edu
   *
   */

function dss_elc_metrics_drush_command() {

  $items = array();

  $items['dss-elc-metrics-cache'] =
    array('description' => dt('Cache Solr results for ELC records within the MongoDB'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => 'drush dss-elc-metrics-cache',
			      ),
	  'aliases' => array(''));

  $items['dss-elc-metrics-solr-index-node'] =
    array('description' => dt('Index a Content Node within Apache Solr'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => 'drush dss-elc-metrics-solr-index-node',
			      ),
	  'aliases' => array(''));

  $items['dss-elc-metrics-solr-index-nodes'] =
    array('description' => dt('Index a set of Content Nodes within Apache Solr'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => 'drush dss-elc-metrics-solr-index-nodes',
			      ),
	  'aliases' => array(''));

  return $items;
  }

function drush_dss_elc_metrics_solr_index_node($nid, $env_id = 'solr') {

  drush_log(dt('Preparing to (re)index the Node ' . $nid), 'ok');
  $node = node_load($nid);
  module_load_include('inc', 'apachesolr',  'apachesolr.index');

  // Retrieve the node to be indexed
  $rows = array( (Object) array('entity_type' => 'node',
				'entity_id' => $node->nid,
				'bundle' => $node->type,
				'status' => $node->status,
				'changed' => $node->changed) );

  $documents = array();

  foreach ($rows as $row) {

    $row_documents = apachesolr_index_entities_document($row, 'node', $env_id);
    $documents = array_merge($documents, $row_documents);
  }

  $indexed = apachesolr_index_send_to_solr($env_id, $documents);
  if ($indexed == FALSE) {

    drush_log(dt('Failed to index the Node ' . $node->nid), 'error');
  } else {
    
    drush_log(dt('Successfully indexed the Node ' . $node->nid), 'success');
  }
}

/**
 * Batch indexing of content nodes
 * Selected by bundle and limits
 *
 */
function drush_dss_elc_metrics_solr_index_nodes($bundle, $env_id = 'solr', $limit = NULL) {

  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', $bundle);

  if(!is_null($limit)) {

    $query->range(0, intval($limit));
  }

  $result = $query->execute();
  if (isset($result['node'])) {

    foreach(array_keys($result['node']) as $nid) {

      drush_dss_elc_metrics_solr_index_node($nid, $env_id);
    }
  }
}

function drush_dss_elc_metrics_cache($view = NULL, $sort_value = NULL) {

  // Make the request to Solr
  $request = array();

  $mongo_collection = _dss_elc_mongodb_get_collection('views_datatables_cache');

  $view_map = array('loans' => array('fields' => array('sort_field_loan_shareholder_label',
						       'sort_field_bib_rel_subject_label',
						       'is_field_loan_duration_value',
						       'sort_field_bib_rel_object_label',
						       'is_field_loan_volumes_loaned_first',
						       'is_changed'),
				     'bundles' => array('loan')
				     ),
		    'items' => array('fields' => array('sort_field_artifact_was_authored_by_label',
						       'sort_label',
						       'is_field_item_volume_migrate',
						       'bundle',
						       'sort_field_item_subject',
						       'is_changed'),
				     'bundles' => array('item', 'book', 'periodical')
				     ),
		    'people' => array('fields' => array('sort_label',
							'is_changed'),
				      'bundles' => array('human')
				      )
		    );

  if(!is_null($view)) {

    $view_map = array($view => $view_map[$view]);

    if(!is_null($sort_value)) {

      $view_map[$view]['fields'] = array($sort_value);
    }
  }

  foreach($view_map as $view => $values) {

    foreach($values['fields'] as $sort_value) {

      $bundles = $values['bundles'];

      foreach(array('asc', 'desc') as $sort_dir) {

	$mongo_key = 'views_datatables_cache_' . $view . '_' . $sort_value . '_' . $sort_dir . '_' . 0;

	$request = array();

	$request['columns'] = array();
	//$request['order']['0']['column'] = $sort_value;
	$request['order']['0']['column'] = array_search($sort_value, $values['fields']);
	$request['order']['0']['dir'] = $sort_dir;
	$request['length'] = 50;
	$request['start'] = 0;

	// Spoof the header for AJAX handling
	$_SERVER['HTTP_X_REQUESTED_WITH'] = 'xmlhttprequest';

	set_time_limit(420);

	drush_log(dt('Caching the DataTables Object for the MongoDB Document ' . $mongo_key . '...'), 'ok');

	list($final_query, $solr_response) = dss_elc_metrics_datatables_query($bundles, $request);

	drush_log(dt('The response to the Solr query was successfully received'), 'success');

	$datatable_json = dss_elc_metrics_datatables_response($solr_response, $bundles);
	
	// Cache the response
	//$mongo_collection->insert(array('view' => $mongo_key, 'data' => $datatable_json));
	$mongo_collection->update(array('view' => $mongo_key),
				  array('view' => $mongo_key, 'data' => $datatable_json),
				  array('upsert' => TRUE));
	drush_log(dt('The DataTables Object was successfully cached in the MongoDB Collection ' . 'views_datatables_cache'), 'success');
      }
    }
  }
}

