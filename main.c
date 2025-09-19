#include "core.h"
#include "csv.h"
#include <pthread.h>

static String8 read_file(Arena* arena, const char* file, u64 size);

typedef struct {
  String8 bible;
  String8 book;
  String8 chapter;
  String8 verse;
  String8 text;
} Verse;

typedef struct {
  Verse* verses;
  usize count;
  Arena* arena;
} ThreadVerses;

typedef struct {
  u8* begin;
  u8* end;
  Arena* arena;
  ThreadVerses* out;
} SliceArg;

typedef struct {
  Arena* main_arena;
  usize offset;
  usize size;
} ThreadArena;

static void push_verse(ThreadVerses* tv, Verse v) {
  Verse* slot = push_one(tv->arena, Verse);
  *slot = v;
  if (tv->count == 0) tv->verses = slot;
  tv->count++;
}

static Verse parse_line(String8 line) {
  u8* p = line.str;
  u8* end = line.str + line.size;
  u8* field_start = p;

  Verse v;

  while (p < end && *p != ':') p++;
  v.bible = str8_range(field_start, p);
  p++; field_start = p;

  while (p < end && *p != ':') p++;
  v.book = str8_range(field_start, p);
  p++; field_start = p;

  while (p < end && *p != ':') p++;
  v.chapter = str8_range(field_start, p);
  p++; field_start = p;

  while (p < end && *p != ':') p++;
  v.verse = str8_range(field_start, p);

  if (p + 1 < end && *p == ':' && *(p + 1) == ':') p += 2;
  field_start = p;

  v.text = str8_range(p, end);
  return v;
}

void* parse_slice_thread(void* arg_) {
  SliceArg* arg = (SliceArg*)arg_;
  Temp temp = scratch_begin(arg->arena);
  u8* p = arg->begin;
  while (p < arg->end) {
    u8* line_end = memchr(p, '\n', arg->end - p);
    if (!line_end) line_end = arg->end;

    String8 line = {p, line_end-p};
    Verse v = parse_line(line);
    push_verse(arg->out, v);

    p = line_end + 1;
  }

  scratch_end(&temp);
  return NULL;
}

static void split_buffer_lines(Arena* arena, String8 string, i32 num_threads,
                               SliceArg* slices, ThreadVerses* outs) {
  usize approx = string.size / num_threads;
  usize start  = 0;

  for (i32 t = 0; t < num_threads; t++) {
    usize end = (t == num_threads - 1) ? string.size : start + approx;
    while (end < string.size && string.str[end] != '\n') end++;
    slices[t].begin = string.str + start;
    slices[t].end   = string.str + end;
    slices[t].arena = arena;
    slices[t].out   = &outs[t];

    start = end + 1;
  }
}

void parse_bible_file_mt(Arena* arena, String8 string, i32 num_threads, ThreadVerses* outs) {
  pthread_t threads[num_threads];
  SliceArg  slices[num_threads];

  for (i32 t = 0; t < num_threads; t++) {
    Arena* thread_arena = arena_alloc(arena, sizeof(Arena));
    arena_init(thread_arena, KB(64));
    outs[t].count  = 0;
    outs[t].verses = NULL;
    outs[t].arena  = arena;
  }

  split_buffer_lines(arena, string, num_threads, slices, outs);

  for (i32 t = 0; t < num_threads; t++) {
    pthread_create(&threads[t], NULL, parse_slice_thread, &slices[t]);
  }
  for (i32 t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }
  
}

i32 main(i32 argc, const char* argv[]) {
  Arena arena = {0};
  arena_init(&arena, MB(8));
  String8 kjv_csv_contents = read_file(&arena, "data/kjv.csv", KB(2));
  CSVDocument kjv_csv = parse_csv(&arena, kjv_csv_contents);
  String8 kjv_txt = read_file(&arena, "data/kjv.txt", MB(5));

  ThreadVerses* outs = push_one(&arena, ThreadVerses);
  outs->arena = &arena;
  parse_bible_file_mt(&arena, kjv_txt, 4, outs);

  for (i32 i = 0; i < outs->count; i++) {
    LOG("%.*s",str8_varg(outs->verses[i].text));
  }

  arena_free(&arena);
  return 0;
}

static String8 read_file(Arena* arena, const char* file, u64 size) {
  char* result;
  char buffer[size];
  i32 ret;
  FILE* fp = fopen(file, "r");

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
