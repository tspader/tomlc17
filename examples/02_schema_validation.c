#define SP_IMPLEMENTATION
#include "sp.h"

#include "toml_schema.h"
#include "tomlc17.h"

// Example 2: Schema validation
// Demonstrates validating TOML against a programmatic schema

s32 main(void) {
  const c8* config_toml = "[database]\n"
                          "host = \"localhost\"\n"
                          "port = 5432\n"
                          "max_connections = 100\n"
                          "username = \"admin\"\n";

  toml_result_t result = toml_parse(config_toml, (int)strlen(config_toml));
  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }

  // Build schema programmatically
  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();

  // Database table schema
  toml_schema_rule_t* db_schema = toml_schema_table();
  toml_schema_add_property(db_schema, SP_LIT("host"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(db_schema, SP_LIT("port"),
                           toml_schema_required(toml_schema_max_int(
                               toml_schema_min_int(toml_schema_int(), 1), 65535)));
  toml_schema_add_property(db_schema, SP_LIT("max_connections"),
                           toml_schema_max_int(toml_schema_min_int(toml_schema_int(), 1), 1000));
  toml_schema_add_property(db_schema, SP_LIT("username"),
                           toml_schema_required(toml_schema_min_length(toml_schema_string(), 1)));

  toml_schema_add_property(root, SP_LIT("database"), toml_schema_required(db_schema));
  toml_schema_set_root(schema, root);

  // Validate
  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);

  if (validation.valid) {
    SP_LOG("✓ Validation passed!");
  } else {
    SP_LOG("✗ Validation failed:");
    sp_dyn_array_for(validation.errors, i) {
      toml_schema_error_t error = validation.errors[i];
      SP_LOG("  {:fg yellow}: {}", SP_FMT_STR(error.path), SP_FMT_STR(error.message));
    }
  }

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);

  return validation.valid ? 0 : 1;
}
