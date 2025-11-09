#include "toml_schema.h"

// Schema builder for spn.toml configuration files
// Based on analysis of spn source code structure

toml_schema_t* spn_schema_create(void) {
  toml_schema_t* schema = toml_schema_new();
  toml_schema_rule_t* root = toml_schema_table();

  // [package] table - REQUIRED
  toml_schema_rule_t* package_schema = toml_schema_table();
  toml_schema_add_property(package_schema, SP_LIT("name"),
                           toml_schema_required(toml_schema_min_length(toml_schema_string(), 1)));
  toml_schema_add_property(package_schema, SP_LIT("version"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(package_schema, SP_LIT("repo"), toml_schema_string());
  toml_schema_add_property(package_schema, SP_LIT("author"), toml_schema_string());
  toml_schema_add_property(package_schema, SP_LIT("maintainer"), toml_schema_string());
  toml_schema_add_property(package_schema, SP_LIT("commit"), toml_schema_string());
  toml_schema_add_property(package_schema, SP_LIT("include"),
                           toml_schema_array(toml_schema_string()));
  toml_schema_add_property(package_schema, SP_LIT("define"),
                           toml_schema_array(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("package"), package_schema);

  // [lib] table - OPTIONAL
  toml_schema_rule_t* lib_schema = toml_schema_table();

  // kinds: array of strings (valid values: "shared", "static")
  sp_da(sp_str_t) lib_kinds = SP_NULLPTR;
  sp_dyn_array_push(lib_kinds, SP_LIT("shared"));
  sp_dyn_array_push(lib_kinds, SP_LIT("static"));

  toml_schema_rule_t* kinds_elem_schema = toml_schema_enum(toml_schema_string(), lib_kinds);
  toml_schema_add_property(lib_schema, SP_LIT("kinds"),
                           toml_schema_array(kinds_elem_schema));
  toml_schema_add_property(lib_schema, SP_LIT("name"), toml_schema_string());
  toml_schema_add_property(root, SP_LIT("lib"), lib_schema);

  // [[profile]] array - OPTIONAL
  toml_schema_rule_t* profile_schema = toml_schema_table();
  toml_schema_add_property(profile_schema, SP_LIT("name"),
                           toml_schema_required(toml_schema_string()));

  // cc: string (valid values: "gcc", "clang", "tcc", "msvc")
  sp_da(sp_str_t) cc_values = SP_NULLPTR;
  sp_dyn_array_push(cc_values, SP_LIT("gcc"));
  sp_dyn_array_push(cc_values, SP_LIT("clang"));
  sp_dyn_array_push(cc_values, SP_LIT("tcc"));
  sp_dyn_array_push(cc_values, SP_LIT("msvc"));
  toml_schema_add_property(profile_schema, SP_LIT("cc"),
                           toml_schema_enum(toml_schema_string(), cc_values));

  // libc: string
  toml_schema_add_property(profile_schema, SP_LIT("libc"), toml_schema_string());

  // standard: string (valid values: "c99", "c11", "c17", etc.)
  toml_schema_add_property(profile_schema, SP_LIT("standard"), toml_schema_string());

  // mode: string (valid values: "debug", "release")
  sp_da(sp_str_t) mode_values = SP_NULLPTR;
  sp_dyn_array_push(mode_values, SP_LIT("debug"));
  sp_dyn_array_push(mode_values, SP_LIT("release"));
  toml_schema_add_property(profile_schema, SP_LIT("mode"),
                           toml_schema_enum(toml_schema_string(), mode_values));

  // language: string alias for standard
  toml_schema_add_property(profile_schema, SP_LIT("language"), toml_schema_string());

  toml_schema_add_property(root, SP_LIT("profile"), toml_schema_array(profile_schema));

  // [[registry]] array - OPTIONAL
  toml_schema_rule_t* registry_schema = toml_schema_table();
  toml_schema_add_property(registry_schema, SP_LIT("name"), toml_schema_string());
  toml_schema_add_property(registry_schema, SP_LIT("location"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(root, SP_LIT("registry"), toml_schema_array(registry_schema));

  // [[bin]] array - OPTIONAL but common
  toml_schema_rule_t* bin_schema = toml_schema_table();
  toml_schema_add_property(bin_schema, SP_LIT("name"),
                           toml_schema_required(toml_schema_string()));
  toml_schema_add_property(bin_schema, SP_LIT("source"),
                           toml_schema_required(toml_schema_min_items(
                               toml_schema_array(toml_schema_string()), 1)));
  toml_schema_add_property(bin_schema, SP_LIT("include"),
                           toml_schema_array(toml_schema_string()));
  toml_schema_add_property(bin_schema, SP_LIT("define"),
                           toml_schema_array(toml_schema_string()));
  toml_schema_add_property(bin_schema, SP_LIT("profile"), toml_schema_string());
  toml_schema_add_property(root, SP_LIT("bin"), toml_schema_array(bin_schema));

  // [deps] table - OPTIONAL
  // Keys are dependency names, values are version strings
  toml_schema_rule_t* deps_schema = toml_schema_table();
  toml_schema_allow_additional(deps_schema);
  toml_schema_additional_schema(deps_schema, toml_schema_string());
  toml_schema_add_property(root, SP_LIT("deps"), deps_schema);

  // [options] table - OPTIONAL
  // Keys are option names, values can be string, int, or bool
  toml_schema_rule_t* options_schema = toml_schema_table();
  toml_schema_allow_additional(options_schema);
  toml_schema_additional_schema(options_schema, toml_schema_any());
  toml_schema_add_property(root, SP_LIT("options"), options_schema);

  // [config] table - OPTIONAL
  // Contains nested tables for dependency configuration
  toml_schema_rule_t* config_schema = toml_schema_table();
  toml_schema_allow_additional(config_schema);

  // Each config.<dep> is a table with arbitrary key-value pairs
  toml_schema_rule_t* config_dep_schema = toml_schema_table();
  toml_schema_allow_additional(config_dep_schema);
  toml_schema_additional_schema(config_dep_schema, toml_schema_any());
  toml_schema_additional_schema(config_schema, config_dep_schema);

  toml_schema_add_property(root, SP_LIT("config"), config_schema);

  // Allow additional top-level properties for extensibility
  root->table_rule.allow_additional = false;

  toml_schema_set_root(schema, root);
  return schema;
}
