<?php
// $Id: similarity_search_similarity.inc,v 1.1.2.4 2009/04/23 16:44:29 scottreynolds Exp $
/**
 * @file similarity_search_similarity
 * lovingly crafted by Scott Reynolds
 *
 * Provides a way to draw similarity based on the Drupal search index
 */

/**
 * node similarity based on the search index
 */
class similarity_search_similarity extends similarity_node {
  /**
   * Implementation of the calculate function
   * cosine similarity of the search index
   */
  public function calculate() {
    // get the possible nids based on limits
    // TODO: implement with better limits
    $created_time = time();
    
    // yuck, can't enforce checkboxes as required. Thus...
    if (empty($this->types)) {
      return;
    }
    foreach($this->types as $node_type) {
      if (empty($type_string)) {
        $type_string = "n.type = '%s'";
      }
      else {
        $type_string .= " OR n.type = '%s'";
      }
    }
    $type_string = "(" . $type_string . ")";
    
    // emulate node_update_index()
    $last = variable_get($this->machine_name . '_cron_last', 0);
    
    $limit = $this->node_limit; // very basic right now
    $args = array($last, $last, $last);
    $args = array_merge($args, $this->types);
    $args[] = $limit;
    $temp_query = "SELECT GREATEST(IF(c.last_comment_timestamp IS NULL, 0, c.last_comment_timestamp), n.changed) as last_change, n.nid, n.created, SQRT(SUM(POW(s.score,2))) as mag FROM {node} n LEFT JOIN {node_comment_statistics} c ON n.nid = c.nid INNER JOIN {search_index} s ON s.type = 'node' AND s.sid = n.nid WHERE n.status = 1 AND ((GREATEST(n.changed, c.last_comment_timestamp) = %d) OR (n.changed > %d OR c.last_comment_timestamp > %d)) AND " . $type_string . " GROUP BY n.nid ORDER BY GREATEST(n.changed, c.last_comment_timestamp) ASC, n.nid ASC";
    
    // create the meta data table for the basic information
    db_query_temporary($temp_query . ' LIMIT 0, %d', $args, 'meta_data');
    db_query("ALTER TABLE {meta_data} ADD PRIMARY KEY(nid)");
    
    // create the table that holds the magnitude information for each nid
    db_query_temporary("SELECT sid as nid, SQRT(SUM(POW(score,2))) as mag FROM {search_index} WHERE type = 'node' GROUP BY sid", "local_mag");
    
    // add in primary keys so its faster
    db_query("ALTER TABLE local_mag ADD PRIMARY KEY(nid)");
    
    // this is to remove the stale data. It removes all the data clearing the way for the inseration
    // With the indexes on both nid1 and nid2 doing two queries signifcantly faster then an OR
    // Investigate a UNION? In MySQL 5.0 OR OR can be translated to UNION by query parser when it would speed it up...
    db_query("DELETE s FROM {" . $this->machine_name . "_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid1");
    db_query("DELETE s FROM {" . $this->machine_name . "_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid2");
    
    // It is actually faster to go nid by nid as then the meta data table will
    // use the PRIMARY KEY. Strange though.. consider reworking
    $nid_result = db_query("SELECT nid FROM {meta_data}");
    while ($nid_object = db_fetch_object($nid_result)) {
      // the big nasty insert query
      db_query("INSERT INTO {". $this->machine_name . "_similarity} " .
        // get the largest of the two nids and put it as nid1
        "SELECT IF (s1.sid > s2.sid, s1.sid, s2.sid) as nid1, IF (s1.sid > s2.sid, s2.sid, s1.sid) as nid2, ".
        // calculate the score here and put the $created_time in the SELECT fields as a const
        "(SUM(s1.score * s2.score) / (md.mag * m2.mag)) as sim, $created_time " .
        // JOIN two search tables on the same words but not the same ids.
        "FROM {meta_data} md INNER JOIN {search_index} s1 ON s1.sid = md.nid AND s1.type = 'node' INNER JOIN {search_index} s2 ON s2.word = s1.word AND s1.type = 'node' AND s2.sid <> s1.sid " .
        "INNER JOIN {local_mag} m2 ON m2.nid = s2.sid " .
        // use the primary key for the first table and group the result set
        "WHERE md.nid = %d AND s1.type = 'node' GROUP BY s1.sid, s2.sid HAVING sim > %f " .
        // ON DUPLICATE KEY update the score value with the new score
        "ON DUPLICATE KEY UPDATE score=VALUES(score)", $nid_object->nid, ($this->min_sim / 100));
    }
    // save the last changed away
    $changed = db_result(db_query("SELECT MAX(last_change) FROM {meta_data}"));
    variable_set($this->machine_name . '_cron_last', $changed);
  }
}
