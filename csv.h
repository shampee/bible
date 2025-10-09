#ifndef CSV_H
#define CSV_H
#include "core.h"

typedef struct CSVLine CSVLine;
struct CSVLine {
  String8 short_name;
  u32  chapter_count;
  String8  long_name;
};

typedef struct CSVDocument CSVDocument;
struct CSVDocument {
  CSVLine* lines;
  u64 line_count;
};

CSVDocument parse_csv(Allocator* allocator, String8 line);

#endif // CSV_H
