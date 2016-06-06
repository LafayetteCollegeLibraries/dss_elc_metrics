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

  //function drush_dss_elc_metrics_solr_clean() {
  $items['dss-elc-metrics-solr-clean'] =
    array('description' => dt('Index a set of Content Nodes within Apache Solr'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => 'drush dss-elc-metrics-solr-index-nodes',
			      ),
	  'aliases' => array(''));

  $items['elc-metrics-report'] =
    array('description' => dt('Generate a report containing the loan metrics for the Easton Library Company Project loan records'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => '',
			      ),
	  'aliases' => array(''));

  $items['elc-metrics-report-ext'] =
    array('description' => dt('Generate a report containing the metrics for the Easton Library Company Project loan records'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => '',
			      ),
	  'aliases' => array(''));

  $items['elc-metrics-report-ext-2'] =
    array('description' => dt('Generate a report containing the metrics for the Easton Library Company Project loan records'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => '',
			      ),
	  'aliases' => array(''));

  $items['dss-elc-metrics-clean'] =
    array('description' => dt('Generate a report containing the metrics for the Easton Library Company Project loan records'),
	  'arguments' => array('arg1' => dt(''),
			       ),
	  'examples' => array(
			      'Standard example' => '',
			      ),
	  'aliases' => array(''));

  return $items;
  }

function drush_dss_elc_metrics_clean($mysql_user,
				     $mysql_database='elc',
				     $mysql_server='localhost',
				     $start_nid = NULL) {

  // apachesolr_index_delete_entity_from_index($env_id, $entity_type, $entity_id)
  //$solr = apachesolr_get_solr('');
  $solr = new DrupalApacheSolrService('http://crete0.lafayette.edu:8080/solr' . '/' . 'elc');

  // $solr->deleteByQuery()

  // search($query

  // First, attempt to verify that the loan exists within the legacy database
  $mysql_pass = drush_prompt(dt('Password'), NULL, TRUE, TRUE);

  $db = new PDO("mysql:host=$mysql_server;dbname=$mysql_database;charset=utf8",
		$mysql_user,
		$mysql_pass,
		array(PDO::MYSQL_ATTR_INIT_COMMAND => "SET NAMES utf8"));

  // Clean the loans
  //
  $query_string = 'bundle:loan';
  $params = array('q' => $query_string,
		  'rows' => 30000,
		  //'sort' => $sort
		  );

  $query = apachesolr_drupal_query(NULL, $params, $sort, '', $solr);
  list($query, $solr_response) = apachesolr_do_query($query);
  $loan_docs = $solr_response->response->docs;

  foreach($loan_docs as $loan_doc) {

    //$sql = "SELECT * FROM loans WHERE shareholder_id= AND book_id=";
    //foreach($db->query("SELECT title FROM books WHERE id=" . $book_record_id . " LIMIT 1") as $book) {

    $loan_nid = $loan_doc->entity_id;

    if(!is_null($start_nid) and intval($loan_nid) < intval($start_nid)) {

      continue;
    }

    /**
     * Clean for Loan records with corrupted Item entries
     *
     */
    $item_nids = $loan_doc->sm_field_bib_rel_object;

    foreach($item_nids as $item_nid) {

      $item_nid = preg_replace('/node\:/','',$item_nid);

      // Retrieve the node
      $item_url = "http://elc.prod.lafayette.edu/node/$item_nid";
      $response = get_headers($item_url, 1);

      $item_node_exists = TRUE;
      if(preg_match('/404/', $response[0])) {

	drush_log(dt('Could not resolve the node for the Item !item_nid.  Deleting...', array('!item_nid' => $item_nid)), 'warning');
	$solr->deleteByQuery("entity_id:$item_nid");
	$item_node_exists = FALSE;
      } elseif(preg_match('/403/', $response[0])) {

	drush_log(dt('Item !item_nid is not published (hence, it should not be analyzed).  Deleting...', array('!item_nid' => $item_nid)), 'warning');
	$solr->deleteByQuery("entity_id:$item_nid");
	$item_node_exists = FALSE;

      } elseif(preg_match('/200/', $response[0])) {

	drush_log(dt('Resolved the node for the Item !item_nid.', array('!item_nid' => $item_nid)), 'ok');

	$_params = array(
			 //'q' => "entity_id:$item_nid",
			 'q' => "sm_field_item_embodies:\"node:$item_nid\"",
			'rows' => 1,
			);
	$_query = apachesolr_drupal_query((string) time(), $_params, $sort, '', $solr);
	list($__query, $_solr_response) = apachesolr_do_query($_query);

	$item_docs = $_solr_response->response->docs;

	if(!empty($item_docs)) {

	  $item_doc = array_shift($item_docs);
	  $item_titles = $item_doc->sm_field_artifact_title;
	  $item_title = array_shift($item_titles);
	  $item_title = preg_replace('/\s\[.+$/', '', $item_title);

	  // Attempt to search for the item
	  $sql = "SELECT * FROM books WHERE title='$item_title'";

	  $records = $db->query($sql);
	  $book_id = NULL;
	  foreach($records as $record) {

	    $book_id = $record['id'];
	  }

	  // Attempt to search for the loan
	  // Refactor into a INNER JOIN?
	  $legacy_record_exists = TRUE;

	  $sql = "SELECT * FROM loans WHERE book_id=$book_id";
	  $loan_records = $db->query($sql);
	  $legacy_record_exists = empty($loan_records);
	  
	} else {

	  drush_log(dt('Could not resolve the node for the Item !item_nid within Solr.  Deleting...', array('!item_nid' => $item_nid)), 'warning');
	  exit(1);
	}
      } else {

	print_r($response[0]);

	drush_log(dt('Could not query the production environment for the node for the Item !item_nid.', array('!item_nid' => $item_nid)), 'error');
	exit(1);
      }
    }

    /**
     * Clean for Loan records with corrupted Shareholder entries
     *
     */
    $shareholder_nids = $loan_doc->sm_field_bib_rel_subject;

    foreach($shareholder_nids as $shareholder_nid) {

      $shareholder_nid = preg_replace('/node\:/','',$shareholder_nid);

      // Retrieve the node
      $shareholder_url = "http://elc.prod.lafayette.edu/node/$shareholder_nid";
      $response = get_headers($shareholder_url, 1);

      $shareholder_node_exists = TRUE;
      if(preg_match('/404/', $response[0])) {

	drush_log(dt('Could not resolve the node for the Shareholder !shareholder_nid.  Deleting...', array('!shareholder_nid' => $shareholder_nid)), 'warning');
	$solr->deleteByQuery("entity_id:$shareholder_nid");
	$shareholder_node_exists = FALSE;
      } elseif(preg_match('/403/', $response[0])) {

	drush_log(dt('Shareholder !shareholder_nid is not published (hence, it should not be analyzed).  Deleting...', array('!shareholder_nid' => $shareholder_nid)), 'warning');
	$solr->deleteByQuery("entity_id:$shareholder_nid");
	$shareholder_node_exists = FALSE;

      } elseif(preg_match('/200/', $response[0])) {

	drush_log(dt('Resolved the node for the Shareholder !shareholder_nid.', array('!shareholder_nid' => $shareholder_nid)), 'ok');

	$_params = array(
			 'q' => "entity_id:$shareholder_nid",
			 //'q' => "sm_field_item_embodies:\"node:$item_nid\"",
			'rows' => 1,
			);
	$_query = apachesolr_drupal_query((string) time(), $_params, $sort, '', $solr);
	list($__query, $_solr_response) = apachesolr_do_query($_query);

	$shareholder_docs = $_solr_response->response->docs;

	if(!empty($shareholder_docs)) {

	  $shareholder_doc = array_shift($shareholder_docs);
	  $shareholder_names = $shareholder_doc->sm_field_person_name;
	  $shareholder_name = array_shift($shareholder_names);

	  // Attempt to search for the shareholder

	  /**
	   * In some cases, representatives are the subject of the loan
	   *
	   */
	  $sql = "SELECT * FROM representatives WHERE name='$shareholder_name'";

	  $records = $db->query($sql);
	  $shareholder_id = NULL;
	  foreach($records as $record) {

	    $shareholder_id = $record['id'];
	  }

	  if( !is_null($shareholder_id) ) {

	    $sql = "SELECT * FROM loans WHERE shareholder_id=$shareholder_id";
	    $loan_records = $db->query($sql);
	    $legacy_record_exists = empty($loan_records);
	  } else {

	    /**
	     * In all other cases, shareholders are the subject of the loan
	     *
	     */
	    $sql = "SELECT * FROM shareholders WHERE name='$shareholder_name'";

	    $records = $db->query($sql);
	    foreach($records as $record) {

	      $shareholder_id = $record['id'];
	    }

	    $sql = "SELECT * FROM loans WHERE shareholder_id=$shareholder_id";
	    $records = $db->query($sql);

	    foreach($records as $record) {

	      $shareholder_id = $record['id'];
	    }

	    $sql = "SELECT * FROM loans WHERE shareholder_id=$shareholder_id";
	    $loan_records = $db->query($sql);
	    $legacy_record_exists = empty($loan_records);
	  }

	} else {

	  drush_log(dt('Could not resolve the node for the Shareholder !shareholder_nid within Solr.  Deleting...', array('!shareholder_nid' => $shareholder_nid)), 'warning');
	  exit(1);
	}
      } else {

	print_r($response[0]);

	drush_log(dt('Could not query the production environment for the node for the Shareholder !shareholder_nid.', array('!shareholder_nid' => $shareholder_nid)), 'error');
	exit(1);
      }
    }

    if(!$item_node_exists) {

      drush_log(dt('Could not resolve the Item node for the Loan !loan_nid.  Deleting...', array('!loan_nid' => $loan_nid)), 'warning');
      $solr->deleteByQuery("entity_id:$loan_nid");
    } elseif(!$shareholder_node_exists) {

      drush_log(dt('Could not resolve the Shareholder node for the Loan !loan_nid.  Deleting...', array('!loan_nid' => $loan_nid)), 'warning');
      $solr->deleteByQuery("entity_id:$loan_nid");
    } elseif($legacy_record_exists) {

      drush_log(dt('Could not identify the record for the Loan !loan_nid within the legacy database.  Deleting...', array('!loan_nid' => $loan_nid)), 'warning');

      exit(1);

      $solr->deleteByQuery("entity_id:$loan_nid");
    } else {

      drush_log(dt('Loan !loan_nid appears to be valid.', array('!loan_nid' => $loan_nid)), 'success');
    }

    $solr->commit();

  }
}

function drush_dss_elc_metrics_elc_metrics_report_ext() {

  module_load_include('inc', 'dss_elc_metrics',  'includes/report');

  $report = dss_elc_metrics_report(array('loan','item'),
				   'field_loan_duration',
				   'field_artifact_title',
				   NULL,
				   10);

  print $report;
}

/**
 *
 */
function drush_dss_elc_metrics_elc_metrics_report() {

  $query_string = 'bundle:item';
  $params = array('q' => $query_string,
		  //'rows' => 30000,
		  'rows' => 3,
		  //'sort' => $sort
		  );

  $solr = apachesolr_get_solr('production_solr_server');

  // function apachesolr_drupal_query($name, array $params = array(), $solrsort = '', $base_path = '', DrupalApacheSolrServiceInterface $solr = NULL, $context = array()) {
  $query = apachesolr_drupal_query(NULL, $params, '', '', $solr);
  list($query, $solr_response) = apachesolr_do_query($query);
  $loan_docs = $solr_response->response->docs;

  foreach($loan_docs as $loan_doc) {
    
    print_r($loan_doc);
  }
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

/**
 *
 */
function drush_dss_elc_metrics_solr_clean() {
  module_load_include('inc', 'apachesolr', 'apachesolr.index');

  $solr = new DrupalApacheSolrService('http://crete0.lafayette.edu:8080/solr' . '/' . 'elc');

  // Clean the loans
  $query_string = 'bundle:loan AND site:"https://elc.prod.lafayette.edu/"';
  $params = array('q' => $query_string,
		  'rows' => 30000,
		  //'sort' => $sort
		  );

  $query = apachesolr_drupal_query(NULL, $params, NULL, '', $solr);
  list($query, $solr_response) = apachesolr_do_query($query);
  $loan_docs = $solr_response->response->docs;

  /**
   * Clean for Loan records with corrupted Item entries
   *
   */
  foreach($loan_docs as $loan_doc) {

    $solr = apachesolr_get_solr('production_solr_server');
    $document_id = apachesolr_document_id($loan_doc->entity_id, $loan_doc->entity_type);
    $query = "id:\"$document_id\" OR sm_parent_document_id:\"$document_id\"";

    try {

      drush_log(dt('Deleting !id from the Apache Solr index...', array('!id' => $document_id)), 'ok');
      $solr->deleteByQuery($query);
      apachesolr_set_last_index_updated('production_solr_server', REQUEST_TIME);

      drush_log(dt('Deleted !id from the Apache Solr index.', array('!id' => $document_id)), 'success');
    } catch (Exception $e) {
      drush_log(dt('Apache Solr: !exception', array('!exception' => $e->getMessage())), 'error');
    }

    $solr->commit();
  }
}

function drush_dss_elc_metrics_cache_loans($nid) {

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
		    );

  $filter_query = '(sm_field_bib_rel_subject:"' . $nid . '" OR sm_field_bib_rel_object:"' . $nid . '")';

  foreach($view_map as $view => $values) {

    foreach($values['fields'] as $sort_value) {

      $bundles = $values['bundles'];

      foreach(array('asc', 'desc') as $sort_dir) {

	$mongo_key = 'views_datatables_cache_' . $view . '_' . $sort_value . '_' . $sort_dir . '_' . preg_replace('/\:/', '_', $nid) . '_' . 0;

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
	
	list($final_query, $solr_response) = dss_elc_metrics_datatables_query($bundles, $request, $filter_query);

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

/**
 * Drush task for caching DataTables
 *
 */
function drush_dss_elc_metrics_data_tables_cache($view = NULL, $sort_value = NULL) {

  // Make the request to Solr
  $request = array();

  list($mongo_collection, $mongo) = _dss_elc_mongodb_get_collection('views_datatables_cache');
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
				     //'bundles' => array('item', 'book', 'periodical')
				     'bundles' => array('item')
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

	foreach(array('50', '500') as $length) {

	  $mongo_key = 'views_datatables_cache_' . $view . '_' . $sort_value . '_' . $sort_dir . '_' . 0 . '_' . $length;

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
	  
	  try {
	    
	    list($final_query, $solr_response) = dss_elc_metrics_datatables_query($bundles, $request);
	    
	    $datatable_json = dss_elc_metrics_datatables_response($solr_response, $bundles);

	    drush_log(dt('The response to the Solr query was successfully received'), 'success');
	  } catch(Exception $e) {
	    
	    drush_log(dt('The Solr query failed: !exception', array('!exception' => $e->getMessage())), 'error');
	  }
	  
	  // Cache the response
	  $mongo_collection->update(array('view' => $mongo_key),
				    array('view' => $mongo_key, 'data' => $datatable_json),
				    array('upsert' => TRUE));
	  drush_log(dt('The DataTables Object was successfully cached in the MongoDB Collection ' . 'views_datatables_cache'), 'success');
	}
      }
    }
  }
}

