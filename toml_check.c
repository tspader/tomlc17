#define SP_IMPLEMENTATION
#include "sp.h"
#include "tomlc17.h"

s32 main(s32 argc, c8** argv) {
  if (argc != 2) {
    SP_LOG("ERR: usage: toml_check <file.toml>");
    return 1;
  }

  toml_result_t result = toml_parse_file_ex(argv[1]);
  if (result.ok) {
    SP_LOG("OK");
    toml_free(result);
    return 0;
  } else {
    SP_LOG("ERR: {}", SP_FMT_CSTR(result.errmsg));
    return 1;
  }
}
