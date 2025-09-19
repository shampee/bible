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
  usize cap;
  Arena* arena;
} ThreadVerses;

typedef struct {
  u8* begin;
  u8* end;
  ThreadVerses* out;
} SliceArg;

static Verse* tv_alloc_array(Arena* arena, usize n) {
  if (n == 0) return NULL;
  // compute bytes carefully
  size_t bytes = n * sizeof(Verse);
  // overflow check
  if (bytes / sizeof(Verse) != n) return NULL;
  return (Verse*)arena_alloc(arena, bytes);
}

static void threadverses_init(ThreadVerses *tv, Arena *arena) {
  tv->verses = NULL;
  tv->count  = 0;
  tv->cap    = 0;
  tv->arena  = arena;
}

static void push_verse(ThreadVerses* tv, Verse v) {
  if (tv->cap == 0) {
    usize new_cap = KB(1);
    Verse* arr = tv_alloc_array(tv->arena, new_cap);
    if (!arr) ERROR("failed to alloc verse array for tv");
    tv->verses = arr;
    tv->cap = new_cap;
  } else if (tv->count >= tv->cap) {
    usize new_cap = tv->cap * 2;
    Verse* new_arr = tv_alloc_array(tv->arena, new_cap);
    if (!new_arr) ERROR("failed to alloc verse array for tv");
    MemoryCopy(new_arr, tv->verses, tv->count * sizeof(Verse));
    tv->verses = new_arr;
    tv->cap = new_cap;
  }

  tv->verses[tv->count++] = v;
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
  ThreadVerses* tv = arg->out;
  Temp temp = scratch_begin(tv->arena);

  u8* p = arg->begin;
  while (p < arg->end) {
    u8* line_end = memchr(p, '\n', arg->end - p);
    if (!line_end) line_end = arg->end;

    String8 line = str8_range(p, line_end);
    Verse v = parse_line(line);
    push_verse(tv, v);

    p = line_end + 1;
  }

  scratch_end(&temp);
  return NULL;
}

static void split_buffer_lines(String8 string, i32 num_threads, SliceArg* slices, ThreadVerses* outs) {
  usize approx = string.size / num_threads;
  usize start  = 0;

  for (i32 t = 0; t < num_threads; t++) {
    usize end = (t == num_threads - 1) ? string.size : start + approx;
    while (end < string.size && string.str[end] != '\n') end++;
    if (end < string.size) end++;
    slices[t].begin = string.str + start;
    slices[t].end   = string.str + end;
    slices[t].out   = &outs[t];

    start = end;
  }
}

void parse_bible_file_mt(Arena* arena, String8 string, i32 num_threads, ThreadVerses* outs) {
  pthread_t threads[num_threads];
  SliceArg  slices[num_threads];



  split_buffer_lines(string, num_threads, slices, outs);

  for (i32 t = 0; t < num_threads; t++) {
    pthread_create(&threads[t], NULL, parse_slice_thread, &slices[t]);
  }

  for (i32 t = 0; t < num_threads; t++) {
    DEBUG("joining thread %d", t);
    pthread_join(threads[t], NULL);
  }
  
}

i32 main(i32 argc, const char* argv[]) {
  Arena arena = {0};
  arena_init(&arena, MB(8));
  /* String8 kjv_csv_contents = read_file(&arena, "data/kjv.csv", KB(2)); */
  /* CSVDocument kjv_csv = parse_csv(&arena, kjv_csv_contents); */
  String8 kjv_txt = read_file(&arena, "data/kjv.txt", MB(5));
  /* String8 kjv_sample_txt = read_file(&arena, "data/kjv_sample.txt", KB(6)); */

  u8 nthreads = 4;
  ThreadVerses* tvs = push_array(&arena, ThreadVerses, nthreads);
  Arena* thread_arenas = push_array(&arena, Arena, nthreads);

  for (i32 t = 0; t < nthreads; t++) {
    arena_init(&thread_arenas[t], KB(64));
    threadverses_init(&tvs[t], &thread_arenas[t]);
  }
  parse_bible_file_mt(&arena, kjv_txt, nthreads, tvs);
  /* parse_bible_file_mt(&arena, kjv_sample_txt, nthreads, tvs); */

  u64 total_count = 0;
  for (i32 t = 0; t < nthreads; t++) {
    /* total_count += tvs[tvi].count; */
    ThreadVerses tv = tvs[t];
    for (i32 i = 0; i < tv.count; i++) {
      LOG("%.*s:%.*s:%.*s:%.*s:%.*s",
          str8_varg(tv.verses[i].bible),
          str8_varg(tv.verses[i].book),
          str8_varg(tv.verses[i].chapter),
          str8_varg(tv.verses[i].verse),
          str8_varg(tv.verses[i].text)
         );

      total_count++;
    }
  }
  DEBUG("total_count: %lu", total_count);
  for (i32 t = 0; t < nthreads; t++)
    arena_free(&thread_arenas[t]);

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
