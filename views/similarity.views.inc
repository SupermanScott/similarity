<?php
// $Id: similarity.views.inc,v 1.1.2.2 2009/03/13 21:14:23 scottreynolds Exp $

/**
 * @file similarity.views.inc
 * lovingly crafted by Scott Reynolds
 *
 * Exposes all similarity objects to views
 */

/**
 * Implementation of hook_views_data().
 */
function similarity_views_data() {
  $data = array();
  $all = similarity_all_similarities(TRUE);
  if (!empty($all)) {
    foreach($all as $sim_obj) {
      $data = array_merge($data, $sim_obj->views_data());
    }
  }
  return $data;
}

/**
 * Implementation of hook_views_handlers()
 */
function similarity_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'similarity') .'/views',
    ),
    'handlers' => array(
      'similarity_views_argument_nid' => array(
        'parent' => 'views_handler_argument_node_nid',
      ),
      'similarity_views_argument_tid' => array(
        'parent' => 'views_handler_argument_numeric',
      ),
      'similarity_views_argument_uid' => array(
        'parent' => 'views_handler_argument_user_uid',
      ),
    ),
  );
}

/**
 * The join class does a basic join and doesn't bind the table to a field
 *
 * TODO: evaluate if this is any good?
 */
class similarity_views_join extends views_join {
  function join($table, &$query) {
    $left = $query->get_table_info($this->left_table);
    $output = " $this->type JOIN {" . $this->table . "}";

    // Tack on the extra.
    if (isset($this->extra)) {
      if (is_array($this->extra)) {
        $extras = array();
        foreach ($this->extra as $info) {
          $extra = '';
          // Figure out the table name. Remember, only use aliases provided
          // if at all possible.
          $join_table = '';
          if (!array_key_exists('table', $info)) {
            $join_table = $table['alias'] . '.';
          }
          elseif (isset($info['table'])) {
            $join_table = $info['table'] . '.';
          }

          // And now deal with the value and the operator.  Set $q to
          // a single-quote for non-numeric values and the
          // empty-string for numeric values, then wrap all values in $q.
          $raw_value = $this->db_safe($info['value']);
          $q = (empty($info['numeric']) ? "'" : '');

          if (is_array($raw_value)) {
            $operator = !empty($info['operator']) ? $info['operator'] : 'IN';
            // Transform from IN() notation to = notation if just one value.
            if (count($raw_value) == 1) {
              $value = $q . array_shift($raw_value) . $q;
              $operator = $operator == 'NOT IN' ? '!=' : '=';
            }
            else {
              $value = "($q" . implode("$q, $q", $raw_value) . "$q)";
            }
          }
          else {
            $operator = !empty($info['operator']) ? $info['operator'] : '=';
            $value = "$q$raw_value$q";
          }
          $extras[] = "$join_table$info[field] $operator $value";
        }

        if ($extras) {
          if (count($extras) == 1) {
            $output .= ' AND ' . array_shift($extras);
          }
          else {
            $output .= ' AND (' . implode(' ' . $this->extra_type . ' ', $extras) . ')';
          }
        }
      }
      else if ($this->extra && is_string($this->extra)) {
        $output .= " AND ($this->extra)";
      }
    }
    return $output;
  }
  
}
