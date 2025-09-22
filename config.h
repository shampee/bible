#ifndef CONFIG_H
#define CONFIG_H
#include "core.h"

typedef enum {
  HELP,
  RANDOM_VERSE,
  LOOKUP,
} CommandType;

typedef struct Config Config;
struct Config {
  String8 bible_path;
  String8 csv_path;
  String8 term;
  u8 nthreads;
  CommandType command;
};

Config config_parse(Arena* arena, OptList* options, usize noptions, String8List arglist);

#endif
