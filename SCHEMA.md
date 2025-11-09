# TOML Schema Validation

## Library Overview

A C library for validating TOML documents against schemas and binding data to structs.

### Components

- `toml_schema` - Schema validation with error accumulation
- `toml_bind` - Type-safe struct binding
- `tomlc17` - TOML parser with tagged union representation

### Basic Usage

```c
// Create schema programmatically
toml_schema_t* schema = toml_schema_new();
toml_schema_rule_t* root = toml_schema_table();

toml_schema_add_property(root, SP_LIT("name"),
  toml_schema_required(toml_schema_string()));
toml_schema_add_property(root, SP_LIT("port"),
  toml_schema_max_int(toml_schema_min_int(toml_schema_int(), 1), 65535));

toml_schema_set_root(schema, root);

// Validate
toml_schema_result_t result = toml_schema_validate(schema, data);
if (!result.valid) {
  sp_dyn_array_for(result.errors, i) {
    // Handle errors: result.errors[i].path, result.errors[i].message
  }
}

toml_schema_result_free(result);
toml_schema_free(schema);
```

### Schema Builders

**Type validators:**
- `toml_schema_string()` - String type
- `toml_schema_int()` - Integer type
- `toml_schema_float()` - Float type
- `toml_schema_bool()` - Boolean type
- `toml_schema_table()` - Table/object type
- `toml_schema_array(element_schema)` - Array type
- `toml_schema_any()` - Any type

**Modifiers (chainable):**
- `toml_schema_required(rule)` - Mark as required
- `toml_schema_min_length(rule, min)` - String min length
- `toml_schema_max_length(rule, max)` - String max length
- `toml_schema_enum(rule, values)` - Enum constraint
- `toml_schema_min_int(rule, min)` - Integer minimum
- `toml_schema_max_int(rule, max)` - Integer maximum
- `toml_schema_min_items(rule, min)` - Array min size
- `toml_schema_max_items(rule, max)` - Array max size

**Table operations:**
- `toml_schema_add_property(table, key, schema)` - Add property
- `toml_schema_allow_additional(rule)` - Allow extra properties
- `toml_schema_additional_schema(rule, schema)` - Schema for extra properties

### Struct Binding

```c
typedef struct {
  sp_str_t host;
  s64 port;
  bool enabled;
} config_t;

toml_binder_t* binder = toml_binder_new(sizeof(config_t));
toml_binder_bind_str(binder, SP_LIT("host"), offsetof(config_t, host));
toml_binder_bind_int(binder, SP_LIT("port"), offsetof(config_t, port));
toml_binder_required(toml_binder_bind_bool(binder, SP_LIT("enabled"),
  offsetof(config_t, enabled)));

config_t config = SP_ZERO_INITIALIZE();
toml_bind_result_t result = toml_binder_bind(binder, data, &config);

toml_binder_free(binder);
```

## SPN Schema

Schema for validating `spn.toml` package manifests.

### Usage

```bash
# Validate a manifest
./build/debug/spn_schema myproject/spn.toml

# Run test suite
./build/debug/test_spn_schema
```

### Schema Structure

#### Required

**[package]**
- `name` - string, min length 1
- `version` - string (semver)

#### Optional

**[package]**
- `repo` - string
- `author` - string
- `maintainer` - string
- `commit` - string
- `include` - array of strings
- `define` - array of strings

**[lib]**
- `kinds` - array of enum ("shared", "static")
- `name` - string

**[[profile]]** (array)
- `name` - string, required
- `cc` - enum ("gcc", "clang", "tcc", "msvc")
- `libc` - string
- `standard` - string
- `mode` - enum ("debug", "release")
- `language` - string

**[[registry]]** (array)
- `name` - string
- `location` - string, required

**[[bin]]** (array)
- `name` - string, required
- `source` - array of strings, required, min 1 item
- `include` - array of strings
- `define` - array of strings
- `profile` - string

**[deps]**
- Arbitrary keys with string values (version specs)

**[options]**
- Arbitrary keys with any type values

**[config]**
- Nested tables with arbitrary keys and any type values

### Examples

Minimal valid manifest:
```toml
[package]
name = "myapp"
version = "1.0.0"
```

Standard application:
```toml
[package]
name = "myapp"
version = "1.0.0"

[deps]
sp = "1.5.1"

[[bin]]
name = "myapp"
source = ["src/main.c"]
include = ["include"]
```

With build profiles:
```toml
[package]
name = "myapp"
version = "1.0.0"

[[profile]]
name = "debug"
cc = "gcc"
mode = "debug"

[[bin]]
name = "myapp"
source = ["main.c"]
profile = "debug"
```

### Validation Rules

**Required fields:**
- `package.name` must be non-empty string
- `package.version` must be present
- `bin[].name` must be present
- `bin[].source` must be non-empty array
- `profile[].name` must be present
- `registry[].location` must be present

**Enum validation:**
- `profile[].cc` must be one of: gcc, clang, tcc, msvc
- `profile[].mode` must be one of: debug, release
- `lib.kinds[]` elements must be one of: shared, static

**Type constraints:**
- All fields must match specified types
- Arrays cannot be empty where min_items = 1

### Test Coverage

23 tests covering:
- Minimal configurations
- Standard configurations
- Comprehensive configurations with all features
- Invalid configurations (missing required fields, wrong types, invalid enums)
- Edge cases (version ranges, mixed option types, nested config)

## Building

```bash
spn build

# Run general tests
./build/debug/test

# Run SPN schema tests
./build/debug/test_spn_schema

# Validate a file
./build/debug/spn_schema path/to/spn.toml
```

## Files

- `src/toml_schema.h/c` - Schema validation engine
- `src/toml_bind.h/c` - Struct binding
- `src/spn_schema_def.c` - SPN schema definition
- `spn_schema.c` - SPN validation CLI tool
- `test_spn_schema.c` - SPN schema test suite
- `test.c` - General library tests
- `examples/` - Usage examples
