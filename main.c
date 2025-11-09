#define SP_IMPLEMENTATION
#include "sp.h"

#include "tomlc17.h"

s32 main(s32 num_args, const c8** args) {
  // useless; just to show that everything works
  struct toml_datum_t datum = SP_ZERO_INITIALIZE();

  SP_LOG("hello, {:fg brightcyan}", SP_FMT_CSTR("world"));
  SP_EXIT_SUCCESS();
}
