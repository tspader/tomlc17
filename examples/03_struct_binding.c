#define SP_IMPLEMENTATION
#include "sp.h"

#include <stddef.h>

#include "toml_bind.h"
#include "tomlc17.h"

// Example 3: Struct binding
// Demonstrates binding TOML data directly to C structs

typedef struct {
  sp_str_t host;
  s64 port;
} server_config_t;

typedef struct {
  sp_str_t name;
  sp_str_t version;
  server_config_t server;
  sp_da(sp_str_t) features;
} app_config_t;

s32 main(void) {
  const c8* config_toml = "name = \"my-app\"\n"
                          "version = \"1.0.0\"\n"
                          "\n"
                          "[server]\n"
                          "host = \"0.0.0.0\"\n"
                          "port = 3000\n"
                          "\n"
                          "features = [\"auth\", \"logging\", \"metrics\"]\n";

  toml_result_t result = toml_parse(config_toml, (int)strlen(config_toml));
  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }

  // Create binders
  toml_binder_t* server_binder = toml_binder_new(sizeof(server_config_t));
  toml_binder_required(toml_binder_bind_str(server_binder, SP_LIT("host"),
                                            offsetof(server_config_t, host)));
  toml_binder_required(toml_binder_bind_int(server_binder, SP_LIT("port"),
                                            offsetof(server_config_t, port)));

  toml_binder_t* app_binder = toml_binder_new(sizeof(app_config_t));
  toml_binder_required(
      toml_binder_bind_str(app_binder, SP_LIT("name"), offsetof(app_config_t, name)));
  toml_binder_required(
      toml_binder_bind_str(app_binder, SP_LIT("version"), offsetof(app_config_t, version)));
  toml_binder_bind_table(app_binder, SP_LIT("server"), offsetof(app_config_t, server),
                         server_binder);
  toml_binder_bind_string_array(app_binder, SP_LIT("features"),
                                offsetof(app_config_t, features));

  // Bind data to struct
  app_config_t config = SP_ZERO_INITIALIZE();
  toml_bind_result_t bind_result = toml_binder_bind(app_binder, result.toptab, &config);

  if (!bind_result.success) {
    SP_LOG("Binding error: {:fg red}", SP_FMT_STR(bind_result.error_message));
    toml_binder_free(server_binder);
    toml_binder_free(app_binder);
    toml_free(result);
    return 1;
  }

  // Use the bound data
  SP_LOG("Application Configuration:");
  SP_LOG("  Name: {}", SP_FMT_STR(config.name));
  SP_LOG("  Version: {}", SP_FMT_STR(config.version));
  SP_LOG("  Server: {}:{}", SP_FMT_STR(config.server.host), SP_FMT_S64(config.server.port));
  SP_LOG("  Features:");
  sp_dyn_array_for(config.features, i) {
    SP_LOG("    - {}", SP_FMT_STR(config.features[i]));
  }

  toml_binder_free(server_binder);
  toml_binder_free(app_binder);
  toml_free(result);

  return 0;
}
