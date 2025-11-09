# TOML Schema Validation Library

A minimal, elegant TOML schema validation and struct binding library in C, built on tomlc17 and sp.h.

## Features

- **Schema Validation**: Define schemas programmatically or via TOML files
- **Struct Binding**: Transform TOML directly into C structs with type safety
- **Error Accumulation**: Collect all validation errors, not just the first
- **Idiomatic C**: Uses sp.h patterns throughout - clean, simple, fast

## Quick Start

### 1. Basic TOML Parsing

```c
const c8* config = "[server]\nhost = \"localhost\"\nport = 8080\n";
toml_result_t result = toml_parse(config, strlen(config));

toml_datum_t host = toml_seek(result.toptab, "server.host");
SP_LOG("Host: {}", SP_FMT_CSTR(host.u.s));

toml_free(result);
```

### 2. Schema Validation

```c
// Build schema
toml_schema_t* schema = toml_schema_new();
toml_schema_rule_t* root = toml_schema_table();

toml_schema_add_property(root, SP_LIT("port"),
  toml_schema_required(
    toml_schema_max_int(
      toml_schema_min_int(toml_schema_int(), 1), 65535)));

toml_schema_set_root(schema, root);

// Validate
toml_schema_result_t result = toml_schema_validate(schema, data);

if (!result.valid) {
  sp_dyn_array_for(result.errors, i) {
    SP_LOG("Error: {}", SP_FMT_STR(result.errors[i].message));
  }
}
```

### 3. Struct Binding

```c
typedef struct {
  sp_str_t host;
  s64 port;
} server_t;

// Create binder
toml_binder_t* binder = toml_binder_new(sizeof(server_t));
toml_binder_bind_str(binder, SP_LIT("host"), offsetof(server_t, host));
toml_binder_bind_int(binder, SP_LIT("port"), offsetof(server_t, port));

// Bind data
server_t config = SP_ZERO_INITIALIZE();
toml_bind_result_t result = toml_binder_bind(binder, data, &config);
```

## API Overview

### Schema Validation

**Core types:**
- `toml_schema_t` - Complete schema definition
- `toml_schema_rule_t` - Individual validation rule
- `toml_schema_result_t` - Validation result with errors

**Rule builders:**
- `toml_schema_string()` - String validator
- `toml_schema_int()` - Integer validator
- `toml_schema_float()` - Float validator
- `toml_schema_bool()` - Boolean validator
- `toml_schema_table()` - Table/object validator
- `toml_schema_array(element_schema)` - Array validator
- `toml_schema_any()` - Accept any type

**Constraints (chainable):**
- `toml_schema_required(rule)` - Mark as required
- `toml_schema_min_length(rule, min)` - String min length
- `toml_schema_max_length(rule, max)` - String max length
- `toml_schema_min_int(rule, min)` - Integer min value
- `toml_schema_max_int(rule, max)` - Integer max value
- `toml_schema_min_items(rule, min)` - Array min size
- `toml_schema_max_items(rule, max)` - Array max size
- `toml_schema_allow_additional(rule)` - Allow extra properties

### Struct Binding

**Core types:**
- `toml_binder_t` - Struct binding definition
- `toml_bind_result_t` - Binding result

**Binders:**
- `toml_binder_bind_str(binder, key, offset)`
- `toml_binder_bind_int(binder, key, offset)`
- `toml_binder_bind_float(binder, key, offset)`
- `toml_binder_bind_bool(binder, key, offset)`
- `toml_binder_bind_string_array(binder, key, offset)`
- `toml_binder_bind_table(binder, key, offset, nested_binder)`
- `toml_binder_required(binder)` - Mark last binding as required

## Building

```bash
cd external/tomlc17
spn build
```

Run tests:
```bash
./build/debug/test
```

Run examples:
```bash
./build/debug/example_01_basic_parsing
./build/debug/example_02_schema_validation
./build/debug/example_03_struct_binding
./build/debug/example_04_validation_errors
```

## Examples

See `examples/` directory for complete, runnable examples:

1. `01_basic_parsing.c` - Basic TOML parsing and navigation
2. `02_schema_validation.c` - Programmatic schema validation
3. `03_struct_binding.c` - Binding TOML to C structs
4. `04_validation_errors.c` - Error accumulation and reporting

## Design Philosophy

**Simplicity**: Minimal API surface, no magic, obvious behavior

**Elegance**: Chainable builders, clear data flow, sp.h idioms

**Correctness**: Accumulate all errors, validate thoroughly, type-safe bindings

**Performance**: Compile schemas once, validate many times, zero-copy where possible

## Architecture

The library has three main components:

1. **tomlc17** - TOML parsing (tagged union representation)
2. **toml_schema** - Schema definition and validation engine
3. **toml_bind** - Struct binding for data transformation

All components use sp.h containers (`sp_da`, `sp_ht`) and conventions throughout.

## License

Based on tomlc17 by CK Tan. See source files for license details.
