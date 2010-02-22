<?php
// $Id: similarity_views_argument_tid.inc,v 1.1.2.1 2009/03/13 21:14:23 scottreynolds Exp $

/**
 * @file similarity.module
 * lovingly crafted by Scott Reynolds
 *
 * Provides the required argument handler for term views. This is important so that it
 * can bind itself to the similarity table properly
 */
class similarity_views_argument_tid extends views_handler_argument_numeric {
  function query() {
    $this->ensure_my_table();
    
    // figure out the base table
    if (empty($this->query->relationships[$this->relationship])) {
       $base_table = $this->query->base_table;
    }
    else {
       $base_table = $this->query->relationships[$this->relationship]['base'];
    }
    
    if (!empty($this->options['break_phrase'])) {
      views_break_phrase($this->argument, $this);
    }
    else {
      $this->value = array($this->argument);
    }
    
    $id = $this->query->set_where_group('OR', 'sim');
    if (count($this->value) > 1) {
      $operator = empty($this->options['not']) ? 'IN' : 'NOT IN';
      $placeholders = implode(', ', array_fill(0, sizeof($this->value), '%d'));
      $this->query->add_where(0, "$this->table_alias.$this->real_field $operator ($placeholders)", $this->value);
    }
    else {
      $operator = empty($this->options['not']) ? '=' : '!=';
      $this->query->add_where(0, "IF($this->table_alias.tid1 = %d, $this->table_alias.tid2, $this->table_alias.tid1) = $base_table.tid", $this->argument);
      $this->query->add_where($id, "$this->table_alias.tid1 $operator %d", $this->argument);
      $this->query->add_where($id, "$this->table_alias.tid2 $operator %d", $this->argument);
    }
  }
}