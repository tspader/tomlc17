#define SP_IMPLEMENTATION
#include "sp.h"

#define UTEST_IMPLEMENTATION
#include "utest.h"

#include "toml_schema.h"
#include "tomlc17.h"

// Forward declaration
toml_schema_t* spn_schema_create(void);

// Helper to validate TOML string
static bool validate_toml_string(const c8* toml_str, toml_schema_t* schema) {
  toml_result_t result = toml_parse(toml_str, (int)strlen(toml_str));
  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return false;
  }

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  bool valid = validation.valid;

  if (!valid) {
    SP_LOG("Validation errors:");
    sp_dyn_array_for(validation.errors, i) {
      SP_LOG("  [{}] {}", SP_FMT_STR(validation.errors[i].path),
             SP_FMT_STR(validation.errors[i].message));
    }
  }

  toml_schema_result_free(validation);
  toml_free(result);
  return valid;
}

// ============================================================================
// Tests for MINIMAL valid configurations
// ============================================================================

UTEST(spn_schema, minimal_valid_config) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, minimal_with_single_binary) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n"
                   "source = [\"main.c\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, minimal_with_deps) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[deps]\n"
                   "sp = \"1.5.1\"\n"
                   "utest = \"1.0.0\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

// ============================================================================
// Tests for STANDARD valid configurations
// ============================================================================

UTEST(spn_schema, standard_application_config) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "author = \"John Doe\"\n"
                   "\n"
                   "[deps]\n"
                   "sp = \"1.5.1\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n"
                   "source = [\"src/main.c\", \"src/utils.c\"]\n"
                   "include = [\"include\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, config_with_profile) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"debug\"\n"
                   "cc = \"gcc\"\n"
                   "mode = \"debug\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n"
                   "source = [\"main.c\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, config_with_library) {
  const c8* toml = "[package]\n"
                   "name = \"mylib\"\n"
                   "version = \"2.0.0\"\n"
                   "\n"
                   "[lib]\n"
                   "kinds = [\"static\", \"shared\"]\n"
                   "name = \"mylib\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"example\"\n"
                   "source = [\"example.c\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

// ============================================================================
// Tests for COMPREHENSIVE valid configurations
// ============================================================================

UTEST(spn_schema, comprehensive_config_all_features) {
  const c8* toml = "[package]\n"
                   "name = \"fullapp\"\n"
                   "version = \"3.2.1\"\n"
                   "repo = \"https://github.com/user/fullapp\"\n"
                   "author = \"Jane Developer\"\n"
                   "maintainer = \"Team Lead\"\n"
                   "include = [\"include\", \"vendor\"]\n"
                   "define = [\"DEBUG=1\", \"FEATURE_X\"]\n"
                   "\n"
                   "[lib]\n"
                   "kinds = [\"static\"]\n"
                   "name = \"fullapp\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"debug\"\n"
                   "cc = \"gcc\"\n"
                   "mode = \"debug\"\n"
                   "standard = \"c11\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"release\"\n"
                   "cc = \"clang\"\n"
                   "mode = \"release\"\n"
                   "\n"
                   "[[registry]]\n"
                   "name = \"local\"\n"
                   "location = \"./vendor\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"app\"\n"
                   "source = [\"src/main.c\", \"src/app.c\"]\n"
                   "include = [\"include\"]\n"
                   "define = [\"APP_VERSION=1\"]\n"
                   "profile = \"release\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"test\"\n"
                   "source = [\"test/test.c\"]\n"
                   "\n"
                   "[deps]\n"
                   "sp = \"1.5.1\"\n"
                   "argparse = \">=1.0.0\"\n"
                   "toml = \"1.2.0\"\n"
                   "\n"
                   "[options]\n"
                   "enable_logging = true\n"
                   "max_threads = 8\n"
                   "output_dir = \"./build\"\n"
                   "\n"
                   "[config.sp]\n"
                   "use_foo = true\n"
                   "bar_count = 42\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, multiple_binaries_and_profiles) {
  const c8* toml = "[package]\n"
                   "name = \"multibin\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"fast\"\n"
                   "cc = \"clang\"\n"
                   "mode = \"release\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"safe\"\n"
                   "cc = \"gcc\"\n"
                   "mode = \"debug\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"server\"\n"
                   "source = [\"server.c\"]\n"
                   "profile = \"fast\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"client\"\n"
                   "source = [\"client.c\"]\n"
                   "profile = \"safe\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"tool\"\n"
                   "source = [\"tool.c\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

// ============================================================================
// Tests for INVALID configurations (should fail validation)
// ============================================================================

UTEST(spn_schema, missing_package_name) {
  const c8* toml = "[package]\n"
                   "version = \"1.0.0\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, missing_package_version) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, empty_package_name) {
  const c8* toml = "[package]\n"
                   "name = \"\"\n"
                   "version = \"1.0.0\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, bin_missing_name) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[bin]]\n"
                   "source = [\"main.c\"]\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, bin_missing_source) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, bin_empty_source_array) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n"
                   "source = []\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, profile_missing_name) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[profile]]\n"
                   "cc = \"gcc\"\n"
                   "mode = \"debug\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, invalid_cc_value) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"custom\"\n"
                   "cc = \"invalid_compiler\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, invalid_mode_value) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[profile]]\n"
                   "name = \"custom\"\n"
                   "mode = \"invalid_mode\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, invalid_lib_kinds) {
  const c8* toml = "[package]\n"
                   "name = \"mylib\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[lib]\n"
                   "kinds = [\"invalid_kind\"]\n"
                   "name = \"mylib\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, registry_missing_location) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[[registry]]\n"
                   "name = \"local\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_FALSE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

// ============================================================================
// Tests for EDGE CASES
// ============================================================================

UTEST(spn_schema, version_with_prerelease) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0-alpha.1\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, deps_with_version_ranges) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[deps]\n"
                   "foo = \">=1.0.0\"\n"
                   "bar = \"^2.0.0\"\n"
                   "baz = \"~1.5.0\"\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, mixed_option_types) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[options]\n"
                   "string_opt = \"value\"\n"
                   "int_opt = 42\n"
                   "bool_opt = true\n"
                   "float_opt = 3.14\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST(spn_schema, multiple_config_sections) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[config.dep1]\n"
                   "opt1 = \"value1\"\n"
                   "opt2 = 123\n"
                   "\n"
                   "[config.dep2]\n"
                   "enable = true\n"
                   "count = 5\n";

  toml_schema_t* schema = spn_schema_create();
  ASSERT_TRUE(validate_toml_string(toml, schema));
  toml_schema_free(schema);
}

UTEST_MAIN()
