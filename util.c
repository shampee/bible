#include "util.h"

String8 read_file(Arena* arena, String8 file, u64 size) {
  char* result;
  char buffer[size];
  i32 ret;
  FILE* fp = fopen((char*)file.str, "r");

  if (!fp) {
    ERROR("failed to open file: %s", file);
    return str8_zero();
  }

  result = push_array(arena, char, size+1);

  ret = fread(result, sizeof(*buffer), size, fp);
  result[ret] = '\0';

  fclose(fp);

  return str8_cstring(result);
}
