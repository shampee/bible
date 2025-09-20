#include "search.h"

SearchResult search_keys(Arena* arena, String8 needle, String8List haystack) {
  SearchResult result;
  result.query_count = haystack.node_count;
  result.queries = push_array(arena, Query, haystack.node_count);

  u64 closest_so_far = 20;
  u64 i = 0;

  for (String8Node* node = haystack.first; node; node = node->next) {
    u64 max_length = Max(needle.size, node->string.size);
    isize dist = levenshtein_myers_word_k(needle, node->string, max_length);
    if (dist < closest_so_far) {
      result.closest = node->string;
      closest_so_far = dist;
    }
    result.queries[i] = (Query){.term=needle, .key=node->string, .dist=dist};
    i++;
  }

  return result;
}
