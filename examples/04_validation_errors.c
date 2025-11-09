#define SP_IMPLEMENTATION
#include "sp.h"

#include "toml_schema.h"
#include "tomlc17.h"

// Example 4: Validation error reporting
// Demonstrates comprehensive error accumulation

s32 main(void) {
  // This TOML has multiple validation errors
  const c8* bad_config = "name = \"ok\"\n"
                         "count = -5\n"       // Should be >= 0
                         "description = \"x\"\n" // Too short
                         // missing required 'version' field
                         "unknown_field = 123\n"; // Unknown field

  toml_result_t result = toml_parse(bad_config, (int)strlen(bad_config));
  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }

  // Build strict schema
  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();

  toml_schema_add_property(root, SP_LIT("name"), toml_schema_required(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("version"), toml_schema_required(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("count"),
                           toml_schema_min_int(toml_schema_int(), 0));
  toml_schema_add_property(root, SP_LIT("description"),
                           toml_schema_min_length(toml_schema_string(), 10));

  // Don't allow additional properties
  root->table_rule.allow_additional = false;

  toml_schema_set_root(schema, root);

  // Validate and collect ALL errors
  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);

  SP_LOG("Validation Report:");
  SP_LOG("Status: {}", validation.valid ? SP_FMT_CSTR("PASS") : SP_FMT_CSTR("FAIL"));
  SP_LOG("Errors found: {}", SP_FMT_U64(sp_dyn_array_size(validation.errors)));

  if (!validation.valid) {
    SP_LOG("\nErrors:");
    sp_dyn_array_for(validation.errors, i) {
      toml_schema_error_t error = validation.errors[i];
      sp_str_t path_display = sp_str_empty(error.path) ? SP_LIT("<root>") : error.path;
      SP_LOG("  [{}] {}", SP_FMT_STR(path_display), SP_FMT_STR(error.message));
    }
  }

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);

  return 0;
}
