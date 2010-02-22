<?php
// $Id: similarity_search_links.inc,v 1.1.2.3 2009/03/13 21:14:23 scottreynolds Exp $

/**
 * @file similarity_search_links
 * lovingly crafted by Scott Reynolds
 *
 * Provides a way to draw similarity based on what nodes link to other nodes
 * basically useful when nodes reference each other. I.E node_refrence cck field
 */

// the search links class
class similarity_search_links extends similarity_node {
  
  /**
   * Override the base calculate function
   */
  function calculate() {
    // get the possible nids based on limits
    // TODO: implement with better limits
    $possible_nids = array();
    $type_string = '';
    $args = array();
    $sim_cache = array();
    foreach($this->types as $node_type) {
      if (empty($type_string)) {
        $type_string = "n.type = '%s'";
      }
      else {
        $type_string .= " OR n.type = '%s'";
      }
    }
    $type_string = "(" . $type_string . ")";
    $db_nids = db_query("SELECT n.nid FROM {node} n WHERE " . $type_string, $this->types);
    while($nid_obj = db_fetch_object($db_nids)) {
      $node_links = db_query("SELECT s.nid FROM {search_node_links} WHERE s.type = 'node' AND s.sid = %d", $nid_obj->nid);
      while ($node_link_row = db_fetch_object($node_links)) {
        // record the link that node has to another node
        $possible_nids[$nid_obj->nid][$node_link_row->nid] = 1;
      }
    }
    
    // set limits
    $last = variable_get($this->machine_name . '_cron_last', 0);
    $limit = $this->node_limit; // very basic right now

    $args[] = $last;
    $args = array_merge($args, $this->types);
    $result = db_query_range('SELECT n.nid, n.changed FROM {node} n WHERE n.changed > %d AND ' . $type_string . ' ORDER BY changed ASC', $args, 0, $limit);

    $last_changed = 0;
   
    while ($changed_obj = db_fetch_object($result)) {
      $current_terms = $possible_nids[$changed_obj->nid];
      $last_changed = $changed_obj->changed;
      
      // process the terms
      if (!empty($current_terms)) {
        foreach($possible_nids as $other_nid => $other_terms) {
          if ($other_nid != $changed_obj->nid) {
            $this->save_similarity($changed_obj->nid, $other_nid, similarity_cosine($current_terms, $other_terms));
          }
        }
      }
      // then insert
      $this->write_similarities();
      if ($last_changed > 0 ) {
        variable_set($this->machine_name . '_cron_last', $last_changed);
      }
    }
  }
}