#ifndef TOML_BIND_H
#define TOML_BIND_H

#include "sp.h"
#include "tomlc17.h"

// Struct binding API - transform TOML data directly into C structs
// Single-file header. Define SP_IMPLEMENTATION before including to get implementation.

typedef enum {
  TOML_BIND_STRING,
  TOML_BIND_INT,
  TOML_BIND_FLOAT,
  TOML_BIND_BOOL,
  TOML_BIND_TABLE,
  TOML_BIND_ARRAY,
  TOML_BIND_STRING_ARRAY,
} toml_bind_type_t;

typedef struct toml_binding_t {
  sp_str_t key;
  toml_bind_type_t type;
  u32 offset;
  bool required;
  struct toml_binder_t* nested_binder;
} toml_binding_t;

typedef struct toml_binder_t {
  sp_da(toml_binding_t) bindings;
  u32 struct_size;
} toml_binder_t;

typedef struct {
  bool success;
  sp_str_t error_message;
} toml_bind_result_t;

toml_binder_t* toml_binder_new(u32 struct_size);
void toml_binder_free(toml_binder_t* binder);

toml_binder_t* toml_binder_bind_str(toml_binder_t* binder, sp_str_t key, u32 offset);
toml_binder_t* toml_binder_bind_int(toml_binder_t* binder, sp_str_t key, u32 offset);
toml_binder_t* toml_binder_bind_float(toml_binder_t* binder, sp_str_t key, u32 offset);
toml_binder_t* toml_binder_bind_bool(toml_binder_t* binder, sp_str_t key, u32 offset);
toml_binder_t* toml_binder_bind_string_array(toml_binder_t* binder, sp_str_t key, u32 offset);

toml_binder_t* toml_binder_bind_table(toml_binder_t* binder, sp_str_t key, u32 offset,
                                      toml_binder_t* nested_binder);

toml_binder_t* toml_binder_required(toml_binder_t* binder);

toml_bind_result_t toml_binder_bind(const toml_binder_t* binder, toml_datum_t data,
                                    void* target_struct);

#ifdef SP_IMPLEMENTATION

#include <stddef.h>
#include <string.h>

toml_binder_t* toml_binder_new(u32 struct_size) {
  toml_binder_t* binder = (toml_binder_t*)sp_os_allocate_memory(sizeof(toml_binder_t));
  *binder = SP_ZERO_STRUCT(toml_binder_t);
  binder->bindings = SP_NULLPTR;
  binder->struct_size = struct_size;
  return binder;
}

void toml_binder_free(toml_binder_t* binder) {
  if (!binder) return;
  sp_dyn_array_free(binder->bindings);
  sp_os_free_memory(binder);
}

static void add_binding(toml_binder_t* binder, sp_str_t key, toml_bind_type_t type, u32 offset) {
  toml_binding_t binding = SP_ZERO_INITIALIZE();
  binding.key = key;
  binding.type = type;
  binding.offset = offset;
  binding.required = false;
  binding.nested_binder = SP_NULLPTR;
  sp_dyn_array_push(binder->bindings, binding);
}

toml_binder_t* toml_binder_bind_str(toml_binder_t* binder, sp_str_t key, u32 offset) {
  add_binding(binder, key, TOML_BIND_STRING, offset);
  return binder;
}

toml_binder_t* toml_binder_bind_int(toml_binder_t* binder, sp_str_t key, u32 offset) {
  add_binding(binder, key, TOML_BIND_INT, offset);
  return binder;
}

toml_binder_t* toml_binder_bind_float(toml_binder_t* binder, sp_str_t key, u32 offset) {
  add_binding(binder, key, TOML_BIND_FLOAT, offset);
  return binder;
}

toml_binder_t* toml_binder_bind_bool(toml_binder_t* binder, sp_str_t key, u32 offset) {
  add_binding(binder, key, TOML_BIND_BOOL, offset);
  return binder;
}

toml_binder_t* toml_binder_bind_string_array(toml_binder_t* binder, sp_str_t key, u32 offset) {
  add_binding(binder, key, TOML_BIND_STRING_ARRAY, offset);
  return binder;
}

toml_binder_t* toml_binder_bind_table(toml_binder_t* binder, sp_str_t key, u32 offset,
                                      toml_binder_t* nested_binder) {
  toml_binding_t binding = SP_ZERO_INITIALIZE();
  binding.key = key;
  binding.type = TOML_BIND_TABLE;
  binding.offset = offset;
  binding.required = false;
  binding.nested_binder = nested_binder;
  sp_dyn_array_push(binder->bindings, binding);
  return binder;
}

toml_binder_t* toml_binder_required(toml_binder_t* binder) {
  if (!sp_dyn_array_empty(binder->bindings)) {
    u32 last_idx = sp_dyn_array_size(binder->bindings) - 1;
    binder->bindings[last_idx].required = true;
  }
  return binder;
}

toml_bind_result_t toml_binder_bind(const toml_binder_t* binder, toml_datum_t data,
                                    void* target_struct) {
  toml_bind_result_t result = SP_ZERO_INITIALIZE();
  result.success = true;
  result.error_message = SP_LIT("");

  if (data.type != TOML_TABLE) {
    result.success = false;
    result.error_message = SP_LIT("data must be a TOML table");
    return result;
  }

  sp_dyn_array_for(binder->bindings, i) {
    toml_binding_t binding = binder->bindings[i];

    c8* key_cstr = sp_str_to_cstr(binding.key);
    toml_datum_t value = toml_get(data, key_cstr);
    sp_free(key_cstr);

    if (value.type == TOML_UNKNOWN && binding.required) {
      result.success = false;
      result.error_message = sp_format("required field \"{}\" is missing", SP_FMT_STR(binding.key));
      return result;
    }

    if (value.type == TOML_UNKNOWN) {
      continue;
    }

    void* field_ptr = (c8*)target_struct + binding.offset;

    switch (binding.type) {
    case TOML_BIND_STRING: {
      if (value.type != TOML_STRING) {
        result.success = false;
        result.error_message =
            sp_format("field \"{}\" expected string, got {}", SP_FMT_STR(binding.key),
                      SP_FMT_U64(value.type));
        return result;
      }
      *(sp_str_t*)field_ptr = sp_str_view(value.u.s);
      break;
    }

    case TOML_BIND_INT: {
      if (value.type != TOML_INT64) {
        result.success = false;
        result.error_message = sp_format("field \"{}\" expected int, got {}", SP_FMT_STR(binding.key),
                                         SP_FMT_U64(value.type));
        return result;
      }
      *(s64*)field_ptr = value.u.int64;
      break;
    }

    case TOML_BIND_FLOAT: {
      if (value.type != TOML_FP64) {
        result.success = false;
        result.error_message =
            sp_format("field \"{}\" expected float, got {}", SP_FMT_STR(binding.key),
                      SP_FMT_U64(value.type));
        return result;
      }
      *(f64*)field_ptr = value.u.fp64;
      break;
    }

    case TOML_BIND_BOOL: {
      if (value.type != TOML_BOOLEAN) {
        result.success = false;
        result.error_message = sp_format("field \"{}\" expected bool, got {}", SP_FMT_STR(binding.key),
                                         SP_FMT_U64(value.type));
        return result;
      }
      *(bool*)field_ptr = value.u.boolean;
      break;
    }

    case TOML_BIND_STRING_ARRAY: {
      if (value.type != TOML_ARRAY) {
        result.success = false;
        result.error_message =
            sp_format("field \"{}\" expected array, got {}", SP_FMT_STR(binding.key),
                      SP_FMT_U64(value.type));
        return result;
      }

      sp_da(sp_str_t)* array_ptr = (sp_da(sp_str_t)*)field_ptr;
      *array_ptr = SP_NULLPTR;

      for (s32 j = 0; j < value.u.arr.size; j++) {
        toml_datum_t elem = value.u.arr.elem[j];
        if (elem.type != TOML_STRING) {
          result.success = false;
          result.error_message =
              sp_format("field \"{}\"[{}] expected string, got {}", SP_FMT_STR(binding.key),
                        SP_FMT_S64(j), SP_FMT_U64(elem.type));
          return result;
        }
        sp_dyn_array_push(*array_ptr, sp_str_view(elem.u.s));
      }
      break;
    }

    case TOML_BIND_TABLE: {
      if (value.type != TOML_TABLE) {
        result.success = false;
        result.error_message =
            sp_format("field \"{}\" expected table, got {}", SP_FMT_STR(binding.key),
                      SP_FMT_U64(value.type));
        return result;
      }

      if (binding.nested_binder) {
        result = toml_binder_bind(binding.nested_binder, value, field_ptr);
        if (!result.success) {
          return result;
        }
      }
      break;
    }

    default:
      result.success = false;
      result.error_message = sp_format("unsupported binding type {}", SP_FMT_U64(binding.type));
      return result;
    }
  }

  return result;
}

#endif // SP_IMPLEMENTATION

#endif // TOML_BIND_H
