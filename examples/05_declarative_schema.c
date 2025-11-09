#define SP_IMPLEMENTATION
#include "sp.h"

#include "toml_schema.h"
#include "tomlc17.h"

// Example 5: Declarative schema from TOML
// Demonstrates loading schema from TOML file and validating nested structures
// with mutually exclusive backends (postgres vs sqlite)

static void validate_file(toml_schema_t* schema, const c8* data_path) {
  SP_LOG("\nValidating: {:fg cyan}", SP_FMT_CSTR(data_path));

  toml_result_t result = toml_parse_file_ex(data_path);
  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return;
  }

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);

  if (validation.valid) {
    SP_LOG("✓ Valid");
  } else {
    SP_LOG("✗ Invalid ({} errors):", SP_FMT_U64(sp_dyn_array_size(validation.errors)));
    sp_dyn_array_for(validation.errors, i) {
      toml_schema_error_t error = validation.errors[i];
      sp_str_t path_display = sp_str_empty(error.path) ? SP_LIT("<root>") : error.path;
      SP_LOG("  [{}] {}", SP_FMT_STR(path_display), SP_FMT_STR(error.message));
    }
  }

  toml_schema_result_free(validation);
  toml_free(result);
}

s32 main(void) {
  // Load schema from TOML file
  toml_schema_t* schema = toml_schema_load(SP_LIT("examples/schema_database.toml"));
  if (!schema) {
    SP_LOG("Failed to load schema");
    return 1;
  }

  SP_LOG("Schema loaded from examples/schema_database.toml");
  SP_LOG("Validates database config with postgres or sqlite backend\n");

  // Validate different configurations
  validate_file(schema, "examples/data_postgres.toml");
  validate_file(schema, "examples/data_sqlite.toml");
  validate_file(schema, "examples/data_invalid.toml");

  toml_schema_free(schema);
  return 0;
}
