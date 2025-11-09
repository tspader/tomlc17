#define SP_IMPLEMENTATION
#include "sp.h"

#define UTEST_IMPLEMENTATION
#include "utest.h"

#include "tomlc17.h"
#include "toml_bind.h"
#include "toml_schema.h"

// ============================================================================
// Foundational TOML parsing tests
// ============================================================================

UTEST(toml, parse_simple_string) {
  const c8* toml = "name = \"hello\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.toptab.type, TOML_TABLE);

  toml_datum_t value = toml_get(result.toptab, "name");
  ASSERT_EQ(value.type, TOML_STRING);
  ASSERT_STREQ(value.u.s, "hello");

  toml_free(result);
}

UTEST(toml, parse_simple_int) {
  const c8* toml = "count = 42\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);

  toml_datum_t value = toml_get(result.toptab, "count");
  ASSERT_EQ(value.type, TOML_INT64);
  ASSERT_EQ(value.u.int64, 42);

  toml_free(result);
}

UTEST(toml, parse_simple_bool) {
  const c8* toml = "enabled = true\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);

  toml_datum_t value = toml_get(result.toptab, "enabled");
  ASSERT_EQ(value.type, TOML_BOOLEAN);
  ASSERT_TRUE(value.u.boolean);

  toml_free(result);
}

UTEST(toml, parse_array_of_strings) {
  const c8* toml = "tags = [\"foo\", \"bar\", \"baz\"]\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);

  toml_datum_t value = toml_get(result.toptab, "tags");
  ASSERT_EQ(value.type, TOML_ARRAY);
  ASSERT_EQ(value.u.arr.size, 3);
  ASSERT_STREQ(value.u.arr.elem[0].u.s, "foo");
  ASSERT_STREQ(value.u.arr.elem[1].u.s, "bar");
  ASSERT_STREQ(value.u.arr.elem[2].u.s, "baz");

  toml_free(result);
}

UTEST(toml, parse_nested_table) {
  const c8* toml = "[server]\nhost = \"localhost\"\nport = 8080\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);

  toml_datum_t server = toml_get(result.toptab, "server");
  ASSERT_EQ(server.type, TOML_TABLE);

  toml_datum_t host = toml_get(server, "host");
  ASSERT_EQ(host.type, TOML_STRING);
  ASSERT_STREQ(host.u.s, "localhost");

  toml_datum_t port = toml_get(server, "port");
  ASSERT_EQ(port.type, TOML_INT64);
  ASSERT_EQ(port.u.int64, 8080);

  toml_free(result);
}

UTEST(toml, seek_nested_key) {
  const c8* toml = "[server]\nhost = \"localhost\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));

  ASSERT_TRUE(result.ok);

  toml_datum_t host = toml_seek(result.toptab, "server.host");
  ASSERT_EQ(host.type, TOML_STRING);
  ASSERT_STREQ(host.u.s, "localhost");

  toml_free(result);
}

// ============================================================================
// Schema validation tests
// ============================================================================

UTEST(schema, validate_string_type) {
  const c8* toml = "name = \"test\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_add_property(root, SP_LIT("name"), toml_schema_required(toml_schema_string()));
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_TRUE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, detect_type_mismatch) {
  const c8* toml = "count = \"not a number\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_add_property(root, SP_LIT("count"), toml_schema_int());
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_FALSE(validation.valid);
  ASSERT_GT(sp_dyn_array_size(validation.errors), 0);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, detect_missing_required_field) {
  const c8* toml = "optional = \"value\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_add_property(root, SP_LIT("required"), toml_schema_required(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("optional"), toml_schema_string());
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_FALSE(validation.valid);
  ASSERT_GT(sp_dyn_array_size(validation.errors), 0);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, validate_string_length_constraints) {
  const c8* toml = "short = \"ab\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_add_property(root, SP_LIT("short"),
                           toml_schema_min_length(toml_schema_string(), 3));
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_FALSE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, validate_int_range_constraints) {
  const c8* toml = "value = 150\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_rule_t* int_rule = toml_schema_int();
  toml_schema_min_int(int_rule, 0);
  toml_schema_max_int(int_rule, 100);
  toml_schema_add_property(root, SP_LIT("value"), int_rule);
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_FALSE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, validate_array_element_types) {
  const c8* toml = "numbers = [1, 2, 3]\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();
  toml_schema_add_property(root, SP_LIT("numbers"), toml_schema_array(toml_schema_int()));
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_TRUE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST(schema, validate_nested_tables) {
  const c8* toml = "[server]\nhost = \"localhost\"\nport = 8080\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();

  toml_schema_rule_t* server_schema = toml_schema_table();
  toml_schema_add_property(server_schema, SP_LIT("host"), toml_schema_string());
  toml_schema_add_property(server_schema, SP_LIT("port"), toml_schema_int());

  toml_schema_add_property(root, SP_LIT("server"), server_schema);
  toml_schema_set_root(schema, root);

  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_TRUE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

// ============================================================================
// Struct binding tests
// ============================================================================

UTEST(bind, simple_struct_binding) {
  typedef struct {
    sp_str_t name;
    s64 count;
    bool enabled;
  } simple_t;

  const c8* toml = "name = \"test\"\ncount = 42\nenabled = true\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_binder_t* binder = toml_binder_new(sizeof(simple_t));
  toml_binder_bind_str(binder, SP_LIT("name"), offsetof(simple_t, name));
  toml_binder_bind_int(binder, SP_LIT("count"), offsetof(simple_t, count));
  toml_binder_bind_bool(binder, SP_LIT("enabled"), offsetof(simple_t, enabled));

  simple_t data = SP_ZERO_INITIALIZE();
  toml_bind_result_t bind_result = toml_binder_bind(binder, result.toptab, &data);

  ASSERT_TRUE(bind_result.success);
  ASSERT_TRUE(sp_str_equal(data.name, SP_LIT("test")));
  ASSERT_EQ(data.count, 42);
  ASSERT_TRUE(data.enabled);

  toml_binder_free(binder);
  toml_free(result);
}

UTEST(bind, string_array_binding) {
  typedef struct {
    sp_da(sp_str_t) tags;
  } config_t;

  const c8* toml = "tags = [\"foo\", \"bar\"]\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_binder_t* binder = toml_binder_new(sizeof(config_t));
  toml_binder_bind_string_array(binder, SP_LIT("tags"), offsetof(config_t, tags));

  config_t data = SP_ZERO_INITIALIZE();
  toml_bind_result_t bind_result = toml_binder_bind(binder, result.toptab, &data);

  ASSERT_TRUE(bind_result.success);
  ASSERT_EQ(sp_dyn_array_size(data.tags), 2);
  ASSERT_TRUE(sp_str_equal(data.tags[0], SP_LIT("foo")));
  ASSERT_TRUE(sp_str_equal(data.tags[1], SP_LIT("bar")));

  toml_binder_free(binder);
  toml_free(result);
}

UTEST(bind, required_field_validation) {
  typedef struct {
    sp_str_t name;
  } config_t;

  const c8* toml = "other = \"value\"\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_binder_t* binder = toml_binder_new(sizeof(config_t));
  toml_binder_required(toml_binder_bind_str(binder, SP_LIT("name"), offsetof(config_t, name)));

  config_t data = SP_ZERO_INITIALIZE();
  toml_bind_result_t bind_result = toml_binder_bind(binder, result.toptab, &data);

  ASSERT_FALSE(bind_result.success);

  toml_binder_free(binder);
  toml_free(result);
}

UTEST(bind, nested_table_binding) {
  typedef struct {
    sp_str_t host;
    s64 port;
  } server_t;

  typedef struct {
    server_t server;
  } config_t;

  const c8* toml = "[server]\nhost = \"localhost\"\nport = 8080\n";
  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  toml_binder_t* server_binder = toml_binder_new(sizeof(server_t));
  toml_binder_bind_str(server_binder, SP_LIT("host"), offsetof(server_t, host));
  toml_binder_bind_int(server_binder, SP_LIT("port"), offsetof(server_t, port));

  toml_binder_t* binder = toml_binder_new(sizeof(config_t));
  toml_binder_bind_table(binder, SP_LIT("server"), offsetof(config_t, server), server_binder);

  config_t data = SP_ZERO_INITIALIZE();
  toml_bind_result_t bind_result = toml_binder_bind(binder, result.toptab, &data);

  ASSERT_TRUE(bind_result.success);
  ASSERT_TRUE(sp_str_equal(data.server.host, SP_LIT("localhost")));
  ASSERT_EQ(data.server.port, 8080);

  toml_binder_free(server_binder);
  toml_binder_free(binder);
  toml_free(result);
}

// ============================================================================
// Integration test - realistic use case
// ============================================================================

UTEST(integration, package_manifest_validation) {
  const c8* toml = "[package]\n"
                   "name = \"myapp\"\n"
                   "version = \"1.0.0\"\n"
                   "\n"
                   "[deps]\n"
                   "sp = \"1.5.1\"\n"
                   "utest = \"1.0.0\"\n"
                   "\n"
                   "[[bin]]\n"
                   "name = \"myapp\"\n"
                   "source = [\"main.c\"]\n";

  toml_result_t result = toml_parse(toml, (int)strlen(toml));
  ASSERT_TRUE(result.ok);

  // Build schema
  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();

  // Package table schema
  toml_schema_rule_t* package_schema = toml_schema_table();
  toml_schema_add_property(package_schema, SP_LIT("name"),
                           toml_schema_required(toml_schema_min_length(toml_schema_string(), 1)));
  toml_schema_add_property(package_schema, SP_LIT("version"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("package"), toml_schema_required(package_schema));

  // Deps table schema (allow any string values)
  toml_schema_rule_t* deps_schema = toml_schema_table();
  toml_schema_allow_additional(deps_schema);
  toml_schema_add_property(root, SP_LIT("deps"), deps_schema);

  // Bin array schema
  toml_schema_rule_t* bin_item_schema = toml_schema_table();
  toml_schema_add_property(bin_item_schema, SP_LIT("name"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(bin_item_schema, SP_LIT("source"),
                           toml_schema_array(toml_schema_string()));

  toml_schema_add_property(root, SP_LIT("bin"), toml_schema_array(bin_item_schema));

  toml_schema_set_root(schema, root);

  // Validate
  toml_schema_result_t validation = toml_schema_validate(schema, result.toptab);
  ASSERT_TRUE(validation.valid);

  toml_schema_result_free(validation);
  toml_schema_free(schema);
  toml_free(result);
}

UTEST_MAIN()
