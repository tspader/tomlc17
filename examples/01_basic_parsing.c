#define SP_IMPLEMENTATION
#include "sp.h"

#include "tomlc17.h"

// Example 1: Basic TOML parsing
// Demonstrates reading and accessing TOML data

s32 main(void) {
  const c8* config_toml = "[server]\n"
                          "host = \"localhost\"\n"
                          "port = 8080\n"
                          "enabled = true\n";

  // Parse TOML
  toml_result_t result = toml_parse(config_toml, (int)strlen(config_toml));

  if (!result.ok) {
    SP_LOG("Parse error: {:fg red}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }

  // Access nested values using toml_seek
  toml_datum_t host = toml_seek(result.toptab, "server.host");
  toml_datum_t port = toml_seek(result.toptab, "server.port");
  toml_datum_t enabled = toml_seek(result.toptab, "server.enabled");

  // Display results
  if (host.type == TOML_STRING) {
    SP_LOG("Host: {:fg cyan}", SP_FMT_CSTR(host.u.s));
  }

  if (port.type == TOML_INT64) {
    SP_LOG("Port: {:fg cyan}", SP_FMT_S64(port.u.int64));
  }

  if (enabled.type == TOML_BOOLEAN) {
    SP_LOG("Enabled: {:fg cyan}", enabled.u.boolean ? SP_FMT_CSTR("true") : SP_FMT_CSTR("false"));
  }

  toml_free(result);
  return 0;
}
