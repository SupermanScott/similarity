<?php

class similarity_views_argument_uid extends views_handler_argument_user_uid {
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
      $this->query->add_where(0, "IF($this->table_alias.uid1 = %d, $this->table_alias.uid2, $this->table_alias.uid1) = $base_table.uid", $this->argument);
      $this->query->add_where($id, "$this->table_alias.uid1 $operator %d", $this->argument);
      $this->query->add_where($id, "$this->table_alias.uid2 $operator %d", $this->argument);
    }
  }
}