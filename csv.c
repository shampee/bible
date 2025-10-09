#include "csv.h"

/* Gen;50;Genesis; */
CSVDocument parse_csv(Allocator* allocator, String8 string) {
  CSVDocument result = {0};

  String8List list  = str8_split(allocator, string, (u8*)"\n", 1, 0);
  result.line_count = list.node_count;
  result.lines      = push_array(allocator, CSVLine, list.node_count);

  i32 i = 0;
  for (String8Node* node = list.first; node; node = node->next) {
    String8List word_list = str8_split(allocator, node->string, (u8*)";", 1, 0);
    String8Node* word = word_list.first;

    result.lines[i].short_name = word->string;
    word = word->next;
    result.lines[i].chapter_count = u32_from_str8(word->string, 10);
    word = word->next;
    result.lines[i].long_name = word->string;
    i++;
  }


  return result;
}
