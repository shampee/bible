#ifndef SEARCH_H
#define SEARCH_H
#include "core.h"

typedef struct Query Query;
struct Query {
  String8 term;
  String8 key;
  isize dist;
};

typedef struct SearchResult SearchResult;
struct SearchResult {
  Query* queries;
  u64 query_count;
  String8 closest;
};

SearchResult search_keys(Arena* arena, String8 needle, String8List haystack);


#endif
