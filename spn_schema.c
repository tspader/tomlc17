#define SP_IMPLEMENTATION
#include "sp.h"

#include "toml_schema.h"
#include "tomlc17.h"

// Forward declaration - implementation in spn_schema_def.c
toml_schema_t* spn_schema_create(void);

s32 main(s32 num_args, const c8** args) {
  if (num_args < 2) {
    SP_LOG("Usage: spn_schema <spn.toml>");
    return 1;
  }

  const c8* toml_path = args[1];

  // Load and parse TOML
  toml_result_t result = toml_parse_file_ex(toml_path);

  if (!result.ok) {
    SP_LOG("{:fg red}Failed to parse TOML: {}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }

  // Create schema
  toml_schema_t* schema = spn_schema_create();

  // Validate
  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);

  // Report results
  SP_LOG("Validation Report for: {:fg cyan}", SP_FMT_CSTR(toml_path));
  SP_LOG("Status: {}", validation.valid ? SP_FMT_CSTR("PASS") : SP_FMT_CSTR("FAIL"));

  if (!validation.valid) {
    SP_LOG("\nErrors found: {:fg red}", SP_FMT_U64(sp_dyn_array_size(validation.errors)));
    sp_dyn_array_for(validation.errors, i) {
      toml_schema_error_t error = validation.errors[i];
      sp_str_t path_display = sp_str_empty(error.path) ? SP_LIT("<root>") : error.path;
      SP_LOG("  {:fg yellow}[{}]: {}", SP_FMT_STR(path_display), SP_FMT_STR(error.message));
    }
  } else {
    SP_LOG("✓ All validation checks passed!");
  }

  // Cleanup
  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);

  return validation.valid ? 0 : 1;
}
