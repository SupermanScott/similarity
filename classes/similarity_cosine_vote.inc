<?php
// $Id: similarity_cosine_vote.inc,v 1.1.2.6 2009/06/03 05:39:36 scottreynolds Exp $

/**
 * @file similarity_cosine_vote
 * lovingly crafted by Scott Reynolds
 *
 * provides a similarity class that draws similarities based on the nodes voting records.
 * that means that nodes that have recieved similar votes from the same users will be
 * similar.
 */


/**
 * node similarity based on the search index
 */
class similarity_cosine_vote extends similarity_node {  
  /**
   * override the form callback
   */
  public function options_form() {
    $form = parent::options_form();
    $tags = db_query("SELECT DISTINCT(tag) FROM {votingapi_vote} WHERE value_type = 'percent'");
    $options[0] = t('Any');
    while ($tag = db_fetch_object($tags)) {
      $options[$tag->tag] = $tag->tag;
    }
    $form['vote_tag'] = array(
      '#type' => 'select',
      '#title' => t('Vote tag'),
      '#description' => t('Votingapi tag that corresponds to the votes cast by users. See !doc', array('!doc' => url('http://drupal.org/node/68857'))),
      '#default_value' => isset($this->vote_tag) ? $this->vote_tag : 0,
      '#options' => $options,
    );
    return $form;
  }
  
  /**
   * Implementation of the calculate function
   * cosine similarity of the search index
   */
  public function calculate() {
    // TODO: shutdown functions and globals
    $type_string = $this->node_type_string();
    $db_nids_query = "SELECT n.nid, SQRT(SUM(POW(vote.value / 100, 2))) as mag FROM {node} n INNER JOIN {votingapi_vote} vote ON vote.content_id = n.nid AND vote.content_type = 'node' WHERE n.nid > %d AND $type_string GROUP BY n.nid ORDER BY vote.content_id LIMIT 0, %d";
    
    $last_nid = variable_get($this->machine_name . '_cron_last_nid', 0);
    $limit = $this->node_limit; // very basic right now
    $args = array($last_nid);
    
    // TODO: tag handling
    /*if (!empty($this->vote_tag)) {
      $args[] = $this->vote_tag;
    }*/
    
    // merge all the args into the query
    $args = array_merge($args, $this->types, array($limit));
    
    // create the meta data temp table. This table serves
    // as a limit and for the init magintude calc for the processed nodes
    db_query_temporary($db_nids_query, $args, 'meta_data');
    db_query("ALTER TABLE {meta_data} ADD PRIMARY KEY (nid)");
    
    // create the temp table for the rest of the magintudes
    db_query_temporary("SELECT v.content_id as nid, SQRT(SUM(POW(v.value / 100, 2))) as mag FROM {votingapi_vote} v WHERE v.content_type = 'node' GROUP BY v.content_id", 'mag_table');
    db_query("ALTER TABLE {mag_table} ADD PRIMARY KEY(nid)");
    
    // this is to remove the stale data. It removes all the data clearing the way for the inseration
    // With the indexes on both nid1 and nid2 doing two queries signifcantly faster then an OR
    // Investigate a UNION? In MySQL 5.0 OR OR can be translated to UNION by query parser when it would speed it up...
    db_query("DELETE s FROM {" . $this->machine_name . "_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid1");
    db_query("DELETE s FROM {" . $this->machine_name . "_similarity} s INNER JOIN {meta_data} m ON m.nid = s.nid2");
    
    // this is the time the similarities are created
    $created_time = time();

    // It is actually faster to go nid by nid as then the meta data table will
    // use the PRIMARY KEY. Strange though.. consider reworking
    $nid_result = db_query("SELECT nid FROM {meta_data}");
    while ($nid_object = db_fetch_object($nid_result)) {
      db_query("INSERT INTO {". $this->machine_name . "_similarity} " .
        // get the largest of the two nids and put it as nid1
        "SELECT IF (v1.content_id > v2.content_id, v1.content_id, v2.content_id) as nid1, IF (v1.content_id > v2.content_id, v2.content_id, v1.content_id) as nid2, ".
        // calculate the score here and put the $created_time in the SELECT fields as a const
        "(SUM((v1.value / 100) * (v2.value / 100)) / (md.mag * m2.mag)) as sim, $created_time " .
        // JOIN two vote tables on the same uid but not the same content_ids.
        "FROM {meta_data} md INNER JOIN {votingapi_vote} v1 ON v1.content_id = md.nid AND v1.content_type = 'node' INNER JOIN {votingapi_vote} v2 ON v2.content_id <> v1.content_id AND v2.content_type = 'node' AND v2.uid = v1.uid " .
        "INNER JOIN {mag_table} m2 ON m2.nid = v2.content_id " .
        // use the primary key for the first table and group the result set
        "WHERE md.nid = %d GROUP BY v1.content_id, v2.content_id HAVING sim > %f " .
        // ON DUPLICATE KEY update the score value with the new score
        "ON DUPLICATE KEY UPDATE score=VALUES(score)", $nid_object->nid, ($this->min_sim / 100));
    }
    
    // check if it had cycled around
    if ($last_nid == db_result(db_query("SELECT MAX(content_id) FROM {votingapi_vote}"))) {
      $last_nid = 0;
    }
    variable_set($this->machine_name . '_cron_last_nid', $last_nid);
  }
}