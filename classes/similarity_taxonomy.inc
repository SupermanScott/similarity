<?php
// $Id: similarity_taxonomy.inc,v 1.1.2.5 2009/04/29 22:28:52 scottreynolds Exp $

/**
 * @file similarity_taxonomy.inc
 * lovingly crafted by Scott Reynolds
 *
 * Provides a way to draw similarity based on the terms on the node. Nodes are
 * similar if they use similar terms
 */

class similarity_taxonomy extends similarity_node {
  
  /**
   * Override the base calculate function
   */
  function calculate() {
    // get the possible nids based on limits
    // TODO: implement with better limits
    $created_time = time();
    $args = array();
    $type_string = $this->node_type_string();
    
    // can't enforce checkboxes....
    if (empty($type_string)) {
      return;
    }
    
    // set limits
    $last = variable_get($this->machine_name . '_cron_last', 0);
    $last = 0;
    $limit = $this->node_limit; // very basic right now

    $args[] = $last;
    $args = array_merge($args, $this->types);
    
    db_query_temporary("SELECT n.nid, n.changed, SQRT(COUNT(t.tid)) as mag FROM {node} n INNER JOIN {term_node} t ON t.nid = n.nid WHERE n.changed > %d AND " . $type_string . " ORDER BY changed ASC LIMIT 0, $limit", $args, "meta_data");
    db_query("ALTER TABLE {meta_data} ADD PRIMARY KEY(nid)");
    $last_changed = 0;
    
    db_query_temporary("SELECT nid, SQRT(COUNT(tid)) as mag FROM {term_node} GROUP BY nid", 'local_mag');
    
    // add in the primary key. makes the rest of the queries much faster
    db_query("ALTER TABLE {local_mag} ADD PRIMARY KEY(nid)");
    
    // Clear out stale records
    db_query("DELETE s FROM {" . $this->machine_name ."_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid1");
    db_query("DELETE s FROM {" . $this->machine_name ."_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid2");
    
    $result = db_query("SELECT nid, changed FROM {meta_data}");
    while ($changed_obj = db_fetch_object($result)) {
      // the big insert query
      db_query("INSERT INTO {". $this->machine_name . "_similarity} ".
               "SELECT IF(t1.nid > t2.nid, t1.nid, t2.nid) as nid1, IF(t1.nid > t2.nid, t2.nid, t1.nid) as nid2, COUNT(t2.nid) / (md.mag * m2.mag) as sim, %d " .
               "FROM {meta_data} md INNER JOIN {term_node} t1 ON t1.nid = md.nid INNER JOIN {term_node} t2 ON t2.tid = t1.tid AND t2.nid <> t1.nid INNER JOIN {local_mag} m2 ON m2.nid = t2.nid WHERE t1.nid = %d GROUP BY nid1, nid2 HAVING sim > %f " .
               "ON DUPLICATE KEY UPDATE score=VALUES(score)", $created_time, $changed_obj->nid, ($this->min_sim / 100));

      if ($changed_obj->changed > $last_changed) {
        $last_changed = $changed_obj->changed;
      }
    }
    
    // save the last changed away for next calculation
    variable_set($this->machine_name . '_cron_last', $last_changed);
  }
}
