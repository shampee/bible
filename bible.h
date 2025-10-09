#ifndef BIBLE_H
#define BIBLE_H
#include "core.h"
#include <pthread.h>

typedef struct Verse Verse;
struct Verse {
  String8 bible;
  String8 book;
  String8 chapter;
  String8 verse;
  String8 text;
};

typedef struct ThreadVerses ThreadVerses;
struct ThreadVerses {
  Verse* verses;
  usize count;
  usize cap;
  Allocator* allocator;
};

typedef struct SliceArg SliceArg;
struct SliceArg {
  u8* begin;
  u8* end;
  ThreadVerses* out;
};

void   threadverses_init(Allocator* allocator, ThreadVerses* tv);
Verse* tv_alloc_array(Allocator* allocator, usize n);
Verse  parse_line(String8 line);
void   push_verse(ThreadVerses* tv, Verse v);
void   print_verse(Allocator* allocator, Verse* v, u64 max_line_length);
void*  parse_slice_thread(void* arg_);
void   split_buffer_lines(String8 string, i32 num_threads, SliceArg* slices, ThreadVerses* outs);
Verse* parse_bible_file(Allocator* allocator, String8 string);
void   parse_bible_file_mt(Allocator* allocator, String8 string, i32 num_threads, ThreadVerses* outs);
#endif // BIBLE_H
