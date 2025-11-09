#ifndef TOML_SCHEMA_H
#define TOML_SCHEMA_H

#include "sp.h"
#include "tomlc17.h"

// Schema validation library for TOML documents
//
// Single-file header. Define SP_IMPLEMENTATION before including to get implementation.
//
// Example usage:
//   toml_schema_t* schema = toml_schema_load(SP_LIT("config.schema.toml"));
//   toml_schema_result_t result = toml_schema_validate(schema, data);
//   if (!result.valid) {
//     // Handle errors
//   }
//   toml_schema_free(schema);

// ============================================================================
// Types
// ============================================================================

typedef enum {
  TOML_SCHEMA_STRING,
  TOML_SCHEMA_INT,
  TOML_SCHEMA_FLOAT,
  TOML_SCHEMA_BOOL,
  TOML_SCHEMA_TABLE,
  TOML_SCHEMA_ARRAY,
  TOML_SCHEMA_ANY,
} toml_schema_type_t;

typedef struct toml_schema_error_t {
  sp_str_t path;
  sp_str_t message;
} toml_schema_error_t;

typedef struct toml_schema_rule_t toml_schema_rule_t;

typedef struct {
  bool has_min_length;
  bool has_max_length;
  u32 min_length;
  u32 max_length;
  sp_da(sp_str_t) enum_values;
  sp_str_t pattern;
} toml_schema_string_rule_t;

typedef struct {
  bool has_min;
  bool has_max;
  s64 min;
  s64 max;
} toml_schema_int_rule_t;

typedef struct {
  bool has_min;
  bool has_max;
  f64 min;
  f64 max;
} toml_schema_float_rule_t;

typedef struct {
  bool has_min_items;
  bool has_max_items;
  u32 min_items;
  u32 max_items;
  toml_schema_rule_t* element_schema;
} toml_schema_array_rule_t;

typedef struct {
  sp_ht(sp_str_t, toml_schema_rule_t*) properties;
  toml_schema_rule_t* additional_properties;
  bool allow_additional;
} toml_schema_table_rule_t;

struct toml_schema_rule_t {
  sp_str_t key;
  toml_schema_type_t type;
  bool required;

  union {
    toml_schema_string_rule_t string_rule;
    toml_schema_int_rule_t int_rule;
    toml_schema_float_rule_t float_rule;
    toml_schema_array_rule_t array_rule;
    toml_schema_table_rule_t table_rule;
  };
};

typedef struct {
  toml_schema_rule_t* root;
  sp_ht(sp_str_t, toml_schema_rule_t*) definitions;
} toml_schema_t;

typedef struct {
  bool valid;
  sp_da(toml_schema_error_t) errors;
} toml_schema_result_t;

// ============================================================================
// API
// ============================================================================

toml_schema_t* toml_schema_load(sp_str_t path);
toml_schema_t* toml_schema_from_data(toml_datum_t schema_data);
void toml_schema_free(toml_schema_t* schema);

toml_schema_result_t toml_schema_validate(const toml_schema_t* schema, toml_datum_t data);
void toml_schema_result_free(toml_schema_result_t result);

toml_schema_t* toml_schema_new(void);
toml_schema_rule_t* toml_schema_string(void);
toml_schema_rule_t* toml_schema_int(void);
toml_schema_rule_t* toml_schema_float(void);
toml_schema_rule_t* toml_schema_bool(void);
toml_schema_rule_t* toml_schema_table(void);
toml_schema_rule_t* toml_schema_array(toml_schema_rule_t* element_schema);
toml_schema_rule_t* toml_schema_any(void);

toml_schema_rule_t* toml_schema_required(toml_schema_rule_t* rule);
toml_schema_rule_t* toml_schema_optional(toml_schema_rule_t* rule);

toml_schema_rule_t* toml_schema_min_length(toml_schema_rule_t* rule, u32 min);
toml_schema_rule_t* toml_schema_max_length(toml_schema_rule_t* rule, u32 max);
toml_schema_rule_t* toml_schema_enum(toml_schema_rule_t* rule, sp_da(sp_str_t) values);
toml_schema_rule_t* toml_schema_pattern(toml_schema_rule_t* rule, sp_str_t pattern);

toml_schema_rule_t* toml_schema_min_int(toml_schema_rule_t* rule, s64 min);
toml_schema_rule_t* toml_schema_max_int(toml_schema_rule_t* rule, s64 max);
toml_schema_rule_t* toml_schema_min_float(toml_schema_rule_t* rule, f64 min);
toml_schema_rule_t* toml_schema_max_float(toml_schema_rule_t* rule, f64 max);

toml_schema_rule_t* toml_schema_min_items(toml_schema_rule_t* rule, u32 min);
toml_schema_rule_t* toml_schema_max_items(toml_schema_rule_t* rule, u32 max);

void toml_schema_add_property(toml_schema_rule_t* table_rule, sp_str_t key, toml_schema_rule_t* property_schema);
toml_schema_rule_t* toml_schema_allow_additional(toml_schema_rule_t* rule);
toml_schema_rule_t* toml_schema_additional_schema(toml_schema_rule_t* rule, toml_schema_rule_t* additional);

void toml_schema_set_root(toml_schema_t* schema, toml_schema_rule_t* root);

#ifdef SP_IMPLEMENTATION

#include <stddef.h>
#include <string.h>

static void add_error(sp_da(toml_schema_error_t) * errors, sp_str_t path, sp_str_t message) {
  toml_schema_error_t error = SP_ZERO_INITIALIZE();
  error.path = path;
  error.message = message;
  sp_dyn_array_push(*errors, error);
}

static sp_str_t type_to_string(toml_type_t type) {
  switch (type) {
  case TOML_STRING: return SP_LIT("string");
  case TOML_INT64: return SP_LIT("int");
  case TOML_FP64: return SP_LIT("float");
  case TOML_BOOLEAN: return SP_LIT("bool");
  case TOML_TABLE: return SP_LIT("table");
  case TOML_ARRAY: return SP_LIT("array");
  case TOML_DATE: return SP_LIT("date");
  case TOML_TIME: return SP_LIT("time");
  case TOML_DATETIME: return SP_LIT("datetime");
  case TOML_DATETIMETZ: return SP_LIT("datetimetz");
  default: return SP_LIT("unknown");
  }
}

static sp_str_t schema_type_to_string(toml_schema_type_t type) {
  switch (type) {
  case TOML_SCHEMA_STRING: return SP_LIT("string");
  case TOML_SCHEMA_INT: return SP_LIT("int");
  case TOML_SCHEMA_FLOAT: return SP_LIT("float");
  case TOML_SCHEMA_BOOL: return SP_LIT("bool");
  case TOML_SCHEMA_TABLE: return SP_LIT("table");
  case TOML_SCHEMA_ARRAY: return SP_LIT("array");
  case TOML_SCHEMA_ANY: return SP_LIT("any");
  }
  return SP_LIT("unknown");
}

static bool type_matches(toml_type_t data_type, toml_schema_type_t schema_type) {
  if (schema_type == TOML_SCHEMA_ANY) return true;

  switch (schema_type) {
  case TOML_SCHEMA_STRING: return data_type == TOML_STRING;
  case TOML_SCHEMA_INT: return data_type == TOML_INT64;
  case TOML_SCHEMA_FLOAT: return data_type == TOML_FP64;
  case TOML_SCHEMA_BOOL: return data_type == TOML_BOOLEAN;
  case TOML_SCHEMA_TABLE: return data_type == TOML_TABLE;
  case TOML_SCHEMA_ARRAY: return data_type == TOML_ARRAY;
  default: return false;
  }
}

static void validate_rule(const toml_schema_rule_t* rule, toml_datum_t data, sp_str_t path,
                          sp_da(toml_schema_error_t) * errors);

static void validate_string(const toml_schema_string_rule_t* rule, toml_datum_t data,
                            sp_str_t path, sp_da(toml_schema_error_t) * errors) {
  SP_ASSERT(data.type == TOML_STRING);

  sp_str_t value = sp_str_view(data.u.s);

  if (rule->has_min_length && value.len < rule->min_length) {
    sp_str_t msg = sp_format("string length {} is less than minimum {}", SP_FMT_U64(value.len),
                             SP_FMT_U64(rule->min_length));
    add_error(errors, path, msg);
  }

  if (rule->has_max_length && value.len > rule->max_length) {
    sp_str_t msg = sp_format("string length {} exceeds maximum {}", SP_FMT_U64(value.len),
                             SP_FMT_U64(rule->max_length));
    add_error(errors, path, msg);
  }

  if (sp_dyn_array_size(rule->enum_values) > 0) {
    bool found = false;
    sp_dyn_array_for(rule->enum_values, i) {
      if (sp_str_equal(value, rule->enum_values[i])) {
        found = true;
        break;
      }
    }
    if (!found) {
      sp_str_builder_t builder = SP_ZERO_INITIALIZE();
      sp_str_builder_append(&builder, SP_LIT("value must be one of: "));
      sp_dyn_array_for(rule->enum_values, i) {
        if (i > 0) sp_str_builder_append(&builder, SP_LIT(", "));
        sp_str_builder_append_fmt(&builder, "\"{}\"", SP_FMT_STR(rule->enum_values[i]));
      }
      add_error(errors, path, sp_str_builder_write(&builder));
    }
  }

  if (!sp_str_empty(rule->pattern)) {
    // Pattern matching not implemented (would need regex library)
  }
}

static void validate_int(const toml_schema_int_rule_t* rule, toml_datum_t data, sp_str_t path,
                         sp_da(toml_schema_error_t) * errors) {
  SP_ASSERT(data.type == TOML_INT64);

  s64 value = data.u.int64;

  if (rule->has_min && value < rule->min) {
    sp_str_t msg =
        sp_format("value {} is less than minimum {}", SP_FMT_S64(value), SP_FMT_S64(rule->min));
    add_error(errors, path, msg);
  }

  if (rule->has_max && value > rule->max) {
    sp_str_t msg =
        sp_format("value {} exceeds maximum {}", SP_FMT_S64(value), SP_FMT_S64(rule->max));
    add_error(errors, path, msg);
  }
}

static void validate_float(const toml_schema_float_rule_t* rule, toml_datum_t data, sp_str_t path,
                           sp_da(toml_schema_error_t) * errors) {
  SP_ASSERT(data.type == TOML_FP64);

  f64 value = data.u.fp64;

  if (rule->has_min && value < rule->min) {
    sp_str_t msg =
        sp_format("value {} is less than minimum {}", SP_FMT_F64(value), SP_FMT_F64(rule->min));
    add_error(errors, path, msg);
  }

  if (rule->has_max && value > rule->max) {
    sp_str_t msg =
        sp_format("value {} exceeds maximum {}", SP_FMT_F64(value), SP_FMT_F64(rule->max));
    add_error(errors, path, msg);
  }
}

static void validate_array(const toml_schema_array_rule_t* rule, toml_datum_t data, sp_str_t path,
                           sp_da(toml_schema_error_t) * errors) {
  SP_ASSERT(data.type == TOML_ARRAY);

  u32 size = (u32)data.u.arr.size;

  if (rule->has_min_items && size < rule->min_items) {
    sp_str_t msg = sp_format("array length {} is less than minimum {}", SP_FMT_U64(size),
                             SP_FMT_U64(rule->min_items));
    add_error(errors, path, msg);
  }

  if (rule->has_max_items && size > rule->max_items) {
    sp_str_t msg = sp_format("array length {} exceeds maximum {}", SP_FMT_U64(size),
                             SP_FMT_U64(rule->max_items));
    add_error(errors, path, msg);
  }

  if (rule->element_schema) {
    for (s32 i = 0; i < data.u.arr.size; i++) {
      sp_str_t elem_path = sp_format("{}[{}]", SP_FMT_STR(path), SP_FMT_S64(i));
      validate_rule(rule->element_schema, data.u.arr.elem[i], elem_path, errors);
    }
  }
}

static void validate_table(const toml_schema_table_rule_t* rule, toml_datum_t data, sp_str_t path,
                           sp_da(toml_schema_error_t) * errors) {
  SP_ASSERT(data.type == TOML_TABLE);

  sp_ht(sp_str_t, bool) seen_properties = SP_NULLPTR;
  sp_ht_set_fns(seen_properties, sp_ht_on_hash_str_key, sp_ht_on_compare_str_key);

  for (s32 i = 0; i < data.u.tab.size; i++) {
    sp_str_t key = sp_str_view(data.u.tab.key[i]);
    toml_datum_t value = data.u.tab.value[i];

    sp_str_t prop_path = sp_str_empty(path) ? key : sp_format("{}.{}", SP_FMT_STR(path), SP_FMT_STR(key));

    toml_schema_rule_t** property_schema = sp_ht_getp(rule->properties, key);

    if (property_schema) {
      sp_ht_insert(seen_properties, key, true);
      validate_rule(*property_schema, value, prop_path, errors);
    } else {
      if (rule->allow_additional) {
        if (rule->additional_properties) {
          validate_rule(rule->additional_properties, value, prop_path, errors);
        }
      } else {
        sp_str_t msg = sp_format("unknown property \"{}\"", SP_FMT_STR(key));
        add_error(errors, prop_path, msg);
      }
    }
  }

  sp_ht_for(rule->properties, it) {
    sp_str_t key = *sp_ht_it_getkp(rule->properties, it);
    toml_schema_rule_t* property_schema = *sp_ht_it_getp(rule->properties, it);

    if (property_schema->required && !sp_ht_getp(seen_properties, key)) {
      sp_str_t prop_path = sp_str_empty(path) ? key : sp_format("{}.{}", SP_FMT_STR(path), SP_FMT_STR(key));
      sp_str_t msg = sp_format("required property \"{}\" is missing", SP_FMT_STR(key));
      add_error(errors, prop_path, msg);
    }
  }

  sp_ht_free(seen_properties);
}

static void validate_rule(const toml_schema_rule_t* rule, toml_datum_t data, sp_str_t path,
                          sp_da(toml_schema_error_t) * errors) {
  if (!type_matches(data.type, rule->type)) {
    sp_str_t msg = sp_format("expected {}, got {}", SP_FMT_STR(schema_type_to_string(rule->type)),
                             SP_FMT_STR(type_to_string(data.type)));
    add_error(errors, path, msg);
    return;
  }

  switch (rule->type) {
  case TOML_SCHEMA_STRING: validate_string(&rule->string_rule, data, path, errors); break;
  case TOML_SCHEMA_INT: validate_int(&rule->int_rule, data, path, errors); break;
  case TOML_SCHEMA_FLOAT: validate_float(&rule->float_rule, data, path, errors); break;
  case TOML_SCHEMA_ARRAY: validate_array(&rule->array_rule, data, path, errors); break;
  case TOML_SCHEMA_TABLE: validate_table(&rule->table_rule, data, path, errors); break;
  case TOML_SCHEMA_BOOL: break;
  case TOML_SCHEMA_ANY: break;
  }
}

toml_schema_result_t toml_schema_validate(const toml_schema_t* schema, toml_datum_t data) {
  toml_schema_result_t result = SP_ZERO_INITIALIZE();
  result.errors = SP_NULLPTR;

  if (!schema || !schema->root) {
    add_error(&result.errors, SP_LIT(""), SP_LIT("schema is null or has no root rule"));
    result.valid = false;
    return result;
  }

  validate_rule(schema->root, data, SP_LIT(""), &result.errors);
  result.valid = sp_dyn_array_empty(result.errors);

  return result;
}

void toml_schema_result_free(toml_schema_result_t result) {
  sp_dyn_array_free(result.errors);
}

toml_schema_t* toml_schema_new(void) {
  toml_schema_t* schema = (toml_schema_t*)sp_os_allocate_memory(sizeof(toml_schema_t));
  *schema = SP_ZERO_STRUCT(toml_schema_t);
  schema->definitions = SP_NULLPTR;
  sp_ht_set_fns(schema->definitions, sp_ht_on_hash_str_key, sp_ht_on_compare_str_key);
  return schema;
}

void toml_schema_set_root(toml_schema_t* schema, toml_schema_rule_t* root) {
  schema->root = root;
}

toml_schema_rule_t* toml_schema_string(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_STRING;
  rule->string_rule.enum_values = SP_NULLPTR;
  rule->string_rule.pattern = SP_LIT("");
  return rule;
}

toml_schema_rule_t* toml_schema_int(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_INT;
  return rule;
}

toml_schema_rule_t* toml_schema_float(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_FLOAT;
  return rule;
}

toml_schema_rule_t* toml_schema_bool(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_BOOL;
  return rule;
}

toml_schema_rule_t* toml_schema_table(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_TABLE;
  rule->table_rule.properties = SP_NULLPTR;
  sp_ht_set_fns(rule->table_rule.properties, sp_ht_on_hash_str_key, sp_ht_on_compare_str_key);
  rule->table_rule.allow_additional = false;
  return rule;
}

toml_schema_rule_t* toml_schema_array(toml_schema_rule_t* element_schema) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_ARRAY;
  rule->array_rule.element_schema = element_schema;
  return rule;
}

toml_schema_rule_t* toml_schema_any(void) {
  toml_schema_rule_t* rule = (toml_schema_rule_t*)sp_os_allocate_memory(sizeof(toml_schema_rule_t));
  *rule = SP_ZERO_STRUCT(toml_schema_rule_t);
  rule->type = TOML_SCHEMA_ANY;
  return rule;
}

toml_schema_rule_t* toml_schema_required(toml_schema_rule_t* rule) {
  rule->required = true;
  return rule;
}

toml_schema_rule_t* toml_schema_optional(toml_schema_rule_t* rule) {
  rule->required = false;
  return rule;
}

toml_schema_rule_t* toml_schema_min_length(toml_schema_rule_t* rule, u32 min) {
  SP_ASSERT(rule->type == TOML_SCHEMA_STRING);
  rule->string_rule.has_min_length = true;
  rule->string_rule.min_length = min;
  return rule;
}

toml_schema_rule_t* toml_schema_max_length(toml_schema_rule_t* rule, u32 max) {
  SP_ASSERT(rule->type == TOML_SCHEMA_STRING);
  rule->string_rule.has_max_length = true;
  rule->string_rule.max_length = max;
  return rule;
}

toml_schema_rule_t* toml_schema_enum(toml_schema_rule_t* rule, sp_da(sp_str_t) values) {
  SP_ASSERT(rule->type == TOML_SCHEMA_STRING);
  rule->string_rule.enum_values = values;
  return rule;
}

toml_schema_rule_t* toml_schema_pattern(toml_schema_rule_t* rule, sp_str_t pattern) {
  SP_ASSERT(rule->type == TOML_SCHEMA_STRING);
  rule->string_rule.pattern = pattern;
  return rule;
}

toml_schema_rule_t* toml_schema_min_int(toml_schema_rule_t* rule, s64 min) {
  SP_ASSERT(rule->type == TOML_SCHEMA_INT);
  rule->int_rule.has_min = true;
  rule->int_rule.min = min;
  return rule;
}

toml_schema_rule_t* toml_schema_max_int(toml_schema_rule_t* rule, s64 max) {
  SP_ASSERT(rule->type == TOML_SCHEMA_INT);
  rule->int_rule.has_max = true;
  rule->int_rule.max = max;
  return rule;
}

toml_schema_rule_t* toml_schema_min_float(toml_schema_rule_t* rule, f64 min) {
  SP_ASSERT(rule->type == TOML_SCHEMA_FLOAT);
  rule->float_rule.has_min = true;
  rule->float_rule.min = min;
  return rule;
}

toml_schema_rule_t* toml_schema_max_float(toml_schema_rule_t* rule, f64 max) {
  SP_ASSERT(rule->type == TOML_SCHEMA_FLOAT);
  rule->float_rule.has_max = true;
  rule->float_rule.max = max;
  return rule;
}

toml_schema_rule_t* toml_schema_min_items(toml_schema_rule_t* rule, u32 min) {
  SP_ASSERT(rule->type == TOML_SCHEMA_ARRAY);
  rule->array_rule.has_min_items = true;
  rule->array_rule.min_items = min;
  return rule;
}

toml_schema_rule_t* toml_schema_max_items(toml_schema_rule_t* rule, u32 max) {
  SP_ASSERT(rule->type == TOML_SCHEMA_ARRAY);
  rule->array_rule.has_max_items = true;
  rule->array_rule.max_items = max;
  return rule;
}

void toml_schema_add_property(toml_schema_rule_t* table_rule, sp_str_t key,
                               toml_schema_rule_t* property_schema) {
  SP_ASSERT(table_rule->type == TOML_SCHEMA_TABLE);
  property_schema->key = key;
  sp_ht_insert(table_rule->table_rule.properties, key, property_schema);
}

toml_schema_rule_t* toml_schema_allow_additional(toml_schema_rule_t* rule) {
  SP_ASSERT(rule->type == TOML_SCHEMA_TABLE);
  rule->table_rule.allow_additional = true;
  return rule;
}

toml_schema_rule_t* toml_schema_additional_schema(toml_schema_rule_t* rule,
                                                   toml_schema_rule_t* additional) {
  SP_ASSERT(rule->type == TOML_SCHEMA_TABLE);
  rule->table_rule.allow_additional = true;
  rule->table_rule.additional_properties = additional;
  return rule;
}

static toml_schema_rule_t* parse_schema_rule(toml_datum_t rule_data);

static toml_schema_type_t parse_type(sp_str_t type_str) {
  if (sp_str_equal(type_str, SP_LIT("string"))) return TOML_SCHEMA_STRING;
  if (sp_str_equal(type_str, SP_LIT("int"))) return TOML_SCHEMA_INT;
  if (sp_str_equal(type_str, SP_LIT("float"))) return TOML_SCHEMA_FLOAT;
  if (sp_str_equal(type_str, SP_LIT("bool"))) return TOML_SCHEMA_BOOL;
  if (sp_str_equal(type_str, SP_LIT("table"))) return TOML_SCHEMA_TABLE;
  if (sp_str_equal(type_str, SP_LIT("array"))) return TOML_SCHEMA_ARRAY;
  if (sp_str_equal(type_str, SP_LIT("any"))) return TOML_SCHEMA_ANY;
  return TOML_SCHEMA_ANY;
}

static toml_schema_rule_t* parse_schema_rule(toml_datum_t rule_data) {
  if (rule_data.type != TOML_TABLE) {
    return toml_schema_any();
  }

  toml_datum_t type_datum = toml_get(rule_data, "$type");
  if (type_datum.type != TOML_STRING) {
    return toml_schema_any();
  }

  toml_schema_type_t type = parse_type(sp_str_view(type_datum.u.s));
  toml_schema_rule_t* rule = SP_NULLPTR;

  switch (type) {
  case TOML_SCHEMA_STRING: rule = toml_schema_string(); break;
  case TOML_SCHEMA_INT: rule = toml_schema_int(); break;
  case TOML_SCHEMA_FLOAT: rule = toml_schema_float(); break;
  case TOML_SCHEMA_BOOL: rule = toml_schema_bool(); break;
  case TOML_SCHEMA_TABLE: rule = toml_schema_table(); break;
  case TOML_SCHEMA_ARRAY: rule = toml_schema_array(SP_NULLPTR); break;
  case TOML_SCHEMA_ANY: rule = toml_schema_any(); break;
  }

  toml_datum_t required_datum = toml_get(rule_data, "$required");
  if (required_datum.type == TOML_BOOLEAN) {
    rule->required = required_datum.u.boolean;
  }

  if (type == TOML_SCHEMA_STRING) {
    toml_datum_t min_len = toml_get(rule_data, "$min_length");
    if (min_len.type == TOML_INT64) {
      toml_schema_min_length(rule, (u32)min_len.u.int64);
    }

    toml_datum_t max_len = toml_get(rule_data, "$max_length");
    if (max_len.type == TOML_INT64) {
      toml_schema_max_length(rule, (u32)max_len.u.int64);
    }

    toml_datum_t enum_datum = toml_get(rule_data, "$enum");
    if (enum_datum.type == TOML_ARRAY) {
      sp_da(sp_str_t) enum_values = SP_NULLPTR;
      for (s32 i = 0; i < enum_datum.u.arr.size; i++) {
        if (enum_datum.u.arr.elem[i].type == TOML_STRING) {
          sp_dyn_array_push(enum_values, sp_str_copy(sp_str_view(enum_datum.u.arr.elem[i].u.s)));
        }
      }
      toml_schema_enum(rule, enum_values);
    }
  } else if (type == TOML_SCHEMA_INT) {
    toml_datum_t min_datum = toml_get(rule_data, "$min");
    if (min_datum.type == TOML_INT64) {
      toml_schema_min_int(rule, min_datum.u.int64);
    }

    toml_datum_t max_datum = toml_get(rule_data, "$max");
    if (max_datum.type == TOML_INT64) {
      toml_schema_max_int(rule, max_datum.u.int64);
    }
  } else if (type == TOML_SCHEMA_FLOAT) {
    toml_datum_t min_datum = toml_get(rule_data, "$min");
    if (min_datum.type == TOML_FP64) {
      toml_schema_min_float(rule, min_datum.u.fp64);
    }

    toml_datum_t max_datum = toml_get(rule_data, "$max");
    if (max_datum.type == TOML_FP64) {
      toml_schema_max_float(rule, max_datum.u.fp64);
    }
  } else if (type == TOML_SCHEMA_ARRAY) {
    toml_datum_t min_items = toml_get(rule_data, "$min_items");
    if (min_items.type == TOML_INT64) {
      toml_schema_min_items(rule, (u32)min_items.u.int64);
    }

    toml_datum_t max_items = toml_get(rule_data, "$max_items");
    if (max_items.type == TOML_INT64) {
      toml_schema_max_items(rule, (u32)max_items.u.int64);
    }

    toml_datum_t element_schema = toml_get(rule_data, "$element");
    if (element_schema.type == TOML_TABLE) {
      rule->array_rule.element_schema = parse_schema_rule(element_schema);
    }
  } else if (type == TOML_SCHEMA_TABLE) {
    for (s32 i = 0; i < rule_data.u.tab.size; i++) {
      const c8* key = rule_data.u.tab.key[i];
      if (key[0] != '$') {
        sp_str_t prop_key = sp_str_copy(sp_str_view(key));
        toml_datum_t prop_schema = rule_data.u.tab.value[i];
        toml_schema_rule_t* property = parse_schema_rule(prop_schema);
        toml_schema_add_property(rule, prop_key, property);
      }
    }

    toml_datum_t allow_additional = toml_get(rule_data, "$allow_additional");
    if (allow_additional.type == TOML_BOOLEAN && !allow_additional.u.boolean) {
      rule->table_rule.allow_additional = false;
    }
  }

  return rule;
}

toml_schema_t* toml_schema_from_data(toml_datum_t schema_data) {
  toml_schema_t* schema = toml_schema_new();
  schema->root = parse_schema_rule(schema_data);
  return schema;
}

toml_schema_t* toml_schema_load(sp_str_t path) {
  c8* path_cstr = sp_str_to_cstr(path);

  toml_result_t result = toml_parse_file_ex(path_cstr);
  sp_free(path_cstr);

  if (!result.ok) {
    SP_LOG("Failed to parse schema file: {}", SP_FMT_CSTR(result.errmsg));
    return SP_NULLPTR;
  }

  toml_schema_t* schema = toml_schema_from_data(result.toptab);
  toml_free(result);

  return schema;
}

void toml_schema_free(toml_schema_t* schema) {
  if (!schema) return;
  sp_ht_free(schema->definitions);
  sp_os_free_memory(schema);
}

#endif // SP_IMPLEMENTATION

#endif // TOML_SCHEMA_H
