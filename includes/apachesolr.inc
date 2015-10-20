<?php

  /**
   * Functionality which requires that the apachesolr Module be installed and enabled
   * @author griffinj@lafayette.edu
   *
   */

/**
 * Query Solr directly for metrics
 * @todo Integrate with and refactor dss_elc_metrics_datatables_query().
 *
 *
 */
function dss_elc_metrics_solr_query($bundles, $query, $rows = 10, $facet = FALSE, $facets = array(), $facet_query = array(), $sort = '', $page = 1) {

  // Simple search query (not fully tested)
  // class_exists('Apache_Solr_Service') ?: spl_autoload_register('_search_api_solr_autoload'); // make sure that SolrPhpClient class is loaded (use only for <=rc3)
  $url = variable_get('apachesolr_host', 'localhost');

  //$solr = new DrupalApacheSolrService('http://santorini0.stage.lafayette.edu:8080/solr' . '/' . 'elc');
  //$solr = new DrupalApacheSolrService('http://santorini0.lafayette.edu:8080/solr' . '/' . 'elc');
  $solr = new DrupalApacheSolrService('http://crete0.lafayette.edu:8080/solr' . '/' . 'elc');
  $solr_field_map = array('loan' => array(
					  /*
					  '' => 'ss_field_loan_shareholder_label',
					  '' => 'ss_field_bib_rel_subject_label',
					  '' => 'is_field_loan_duration_value',
					  '' => 'ss_field_bib_rel_object_label',
					  '' => 'is_field_loan_volumes_loaned_first',
					  */
					  //'' => 'is_changed'
					  ),
			  'item' => array(
					  /*
					  '' => 'ss_field_artifact_was_authored_by_label',
					  '' => 'label',
					  '' => 'is_field_item_volume_migrate',
					  '' => 'ss_field_artifact_type_constant',
					  '' => 'ss_field_item_subject',
					  */
					  //'' => 'is_changed'
					  ),
			  'human' => array(
					   'field_human_gender' => 'bm_field_human_gender',
					   'field_person_type' => 'sm_vid_Person_Types'
					   //'' => 'label',
					   //'' => 'is_changed'
					   )
			  );

  // Handling for single or multiple bundles
  $first_bundle = $bundles[0];

  $bundle_query = array_map(function($bundle) {

      return 'bundle:' . $bundle;
    }, $bundles);

  if(count($bundle_query) == 1) {

    $bundle_query = $bundle_query[0];
  } else {

    //$bundle_query = '(' . implode(' OR ', $bundle_query) . ')';
    $bundle_query = 'bundle:(' . implode(' OR ', $bundles) . ')';
  }

  //! @todo Review and refactor (or remove)
  // Build the field-scoped search
  /*
  $query = array();

  foreach($request['columns'] as $i => $search) {

    if(trim($search['search']['value']) != '' ) {
	    
      $query[] = $solr_field_map[$first_bundle][$i] . ':' . trim($search['search']['value']) . '*';
    }
  }
  */

  //! @todo Review and refactor (or remove)
  /*
  // Replace the bundle field names with the appropriate Solr index field names
  foreach($query as &$query_field) {

    foreach($solr_field_map[$first_bundle] as $bundle_field => $solr_field) {

      //print $bundle_field;
      //$query_field = preg_replace('/' . $bundle_field . '/', $solr_field, $query_field);
    }
  }
  */

  foreach($facets as &$facet_field) {

    foreach($solr_field_map[$first_bundle] as $bundle_field => $solr_field) {

      $facet_field = preg_replace('/' . $bundle_field . '/', $solr_field, $facet_field);
    }
  }

  // For aggregating and storing the total number of records
  $is_unfiltered_query = empty($query);

  $query_string = $bundle_query;
  if(!empty($query) ) {

    $query_string .= ' AND ';
  }
  $query_string .= implode(' AND ', $query);

  // Handling for sorting
  
  /*
  // Retrieve the name of the Solr field
  $solr_field = $solr_field_map[$first_bundle][intval($request['order']['0']['column'])];

  // If this is a "label" field or a single string field type...
  if($solr_field == 'ss_field_artifact_type_constant') {

    $sort = 'bundle' . ' ' . $request['order']['0']['dir'];

  } elseif($solr_field == 'label') {

    $sort = 'sort_' . $solr_field . ' ' . $request['order']['0']['dir'];

  } elseif(preg_match('/^ss_(.+)/', $solr_field, $m)) {

    // ...then apachesolr's default copyField should be used
    $sort = 'sort_' . $m[1] . ' ' . $request['order']['0']['dir'];

  } else {

    // Otherwise, simply attempt to sort on the field name
    $sort = $solr_field . ' ' . $request['order']['0']['dir'];
  }
  */

  $params = array('q' => $query_string,
		  //'rows' => $request['length'],
		  'rows' => $rows,
		  'sort' => $sort
		  );

  if($facet) {

    $params['facet'] = $facet;

    if(!empty($facet_query)) {

      $params['fq'] = $facet_query;
    }

    // Sanitize input (?)
    $params = array_merge($params, $facets);
  }

  // This is the object that knows about the query coming from the user.
  /*
  $query_name = implode('_', array('views_datatables', $first_bundle, $query_string, $sort));
  $query_name = preg_replace('/s/', '_', $query_name);
  */
  $query_name = implode('_', array('dss_elc_metrics', 'solr', $first_bundle, $query_string, $facet, $facets, $sort, $page));
  //$query_name = preg_replace('/\s/', '_', $query_name);
  $query = apachesolr_drupal_query($query_name, $params, $sort, '', $solr);

  // Work-around for the apachesolr Module
  //! @todo Document and pose question to the maintainers of the Module
  //
  if(empty($facet_query)) {

    $query->removeParam('fq');
  }

  // Handling for the pagination of responses
  //$query->page = $request['start'] / ($request['length'] / 5);
  //$query->page = $page;

  list($query, $solr_response) = apachesolr_do_query($query);

  /*
  if($is_unfiltered_query) {

    $total_records = variable_set('dss_elc_metrics_views_datatables_' . $first_bundle . '_total_records', $solr_response->response->numFound);
  }
  */

  return array($query, $solr_response);
}

/**
 * Query using the DataTables GET request
 * @author griffinj@lafayette.edu
 *
 */
function dss_elc_metrics_datatables_query($bundles, $request, $filter_query = NULL) {

  // Simple search query (not fully tested)
  // class_exists('Apache_Solr_Service') ?: spl_autoload_register('_search_api_solr_autoload'); // make sure that SolrPhpClient class is loaded (use only for <=rc3)
  $url = variable_get('apachesolr_host', 'localhost');

  //$solr = new DrupalApacheSolrService('http://santorini0.lafayette.edu:8080/solr' . '/' . 'elc');
  $solr = new DrupalApacheSolrService('http://crete0.lafayette.edu:8080/solr' . '/' . 'elc');
  $solr_field_map = array('loan' => array(

					  //'sort_field_loan_shareholder_label',
					  'sort_field_bib_rel_subject_label',
					  'sort_field_loan_shareholder_label',
					  'is_field_loan_duration_value',
					  'sort_field_bib_rel_object_label',
					  'is_field_loan_volumes_loaned_first',
					  'is_changed'),
			  'item' => array('sort_field_artifact_was_authored_by_label',
					  'label',
					  'is_field_item_volume_migrate',
					  //'im_field_item_volume_migrate',
					  //'ss_field_artifact_type_constant',
					  'bundle_name',
					  //'ss_field_item_subject',
					  'sort_field_item_subject',
					  'is_changed'),
			  'human' => array('label',
					   'is_changed')
			  );

  $solr_num_fields = array('is_field_loan_volumes_loaned_first',
			   'im_field_item_volume_migrate'
			   );

  // Handling for single or multiple bundles
  $first_bundle = $bundles[0];

  /**
   * Crafting the Solr/Lucene query
   *
   */
  $bundle_query = array_map(function($bundle) {

      return 'bundle:' . $bundle;
    }, $bundles);

  if(count($bundle_query) == 1) {

    $bundle_query = $bundle_query[0];
  } else {

    $bundle_query = '(' . implode(' OR ', $bundle_query) . ')';
  }

  // Extended for the filtration (or inclusion) of authors
  // Resolves EDDC-443
  if($first_bundle == 'human' and
     (!array_key_exists('authors', $request) or
      $request['authors'] == 'false')) {

    $bundle_query .= ' AND -sm_vid_Person_Types:Author';
  }
  
  $query = array();

  foreach($request['columns'] as $i => $search) {

    $search['search']['value'] = trim($search['search']['value']);

    if(trim($search['search']['value']) != '' ) {

      // Work-around for integer values
      // Resolves EDDC-444
      if(in_array($solr_field_map[$first_bundle][$i], $solr_num_fields)) {

	$query_field_values = preg_split('/[\s,]/', trim($search['search']['value']) );

	$query_fields = array();
	foreach($query_field_values as $value) {

	  $query_fields[] = $solr_field_map[$first_bundle][$i] . ':' . $value;
	}

	$query[] = implode(' OR ', $query_fields);
      } else {

	// Work-around for the absence of a sort_bundle_name field
	// Resolves EDDC-445
	//! @todo Resolve properly by modifying the Apache Solr indexing process (EDDC-446)
	if($solr_field_map[$first_bundle][$i] == 'bundle_name') {
	  
	  $search['search']['value'] = ucfirst($search['search']['value']);
	}
	    
	$query[] = $solr_field_map[$first_bundle][$i] . ':*' . trim($search['search']['value']) . '*';
      }
    }
  }

  // For aggregating and storing the total number of records
  $is_unfiltered_query = empty($query);

  /*
   * In order to ensure that NULL values are filtered
   */
  $filter_not_null = '+' . $solr_field_map[ $first_bundle ][0] . ':[* TO *]';
  $query[] = $filter_not_null;

  $query_string = $bundle_query;

  if(!empty($query) ) {

    $query_string .= ' AND ';
  }
  $query_string .= implode(' AND ', $query);

  /**
   * Sorting functionality
   *
   */
  // Retrieve the name of the Solr field
  $solr_field = $solr_field_map[$first_bundle][intval($request['order']['0']['column'])];

  // If this is a "label" field or a single string field type...
  if($solr_field == 'ss_field_artifact_type_constant') {

    $sort = 'bundle' . ' ' . $request['order']['0']['dir'];

  } elseif($solr_field == 'label') {

    $sort = 'sort_' . $solr_field . ' ' . $request['order']['0']['dir'];

  } elseif(preg_match('/^ss_(.+)/', $solr_field, $m)) {

    // ...then apachesolr's default copyField should be used
    $sort = 'sort_' . $m[1] . ' ' . $request['order']['0']['dir'];

  } else {

    // Otherwise, simply attempt to sort on the field name
    $sort = $solr_field . ' ' . $request['order']['0']['dir'];
  }

  // For searches scoped to the site
  $query_string .= ' AND site:"' . 'https://elc.prod.lafayette.edu/' . '"';

  $params = array('q' => $query_string,
		  //'rows' => 100000,
		  //'rows' => ($request['length'] / 5),
		  'rows' => $request['length'],

		  //'start' => $request['start'],
		  'sort' => $sort
		  );

  // Handling for related nodes
  if(!is_null($filter_query)) {

    $params['fq'] = $filter_query;
  }

  // This is the object that knows about the query coming from the user.
  $query_name = implode('_', array('views_datatables', $first_bundle, $query_string, $sort));
  $query_name = preg_replace('/s/', '_', $query_name);
  $query = apachesolr_drupal_query($query_name, $params, $sort, '', $solr);

  //$query = apachesolr_drupal_query('apachesolr', $params, $sort, '', $solr);

  //print_r($params);
  //exit(1);

  //$query->page = $request['start'] / ($request['length'] / 5);
  $query->page = $request['start'] / ($request['length']);

  list($query, $solr_response) = apachesolr_do_query($query);

  if($is_unfiltered_query) {

    $total_records = variable_set('dss_elc_metrics_views_datatables_' . $first_bundle . '_total_records', $solr_response->response->numFound);
  }

  return array($query, $solr_response);
}
