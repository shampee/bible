#include "bible.h"

void threadverses_init(ThreadVerses *tv, Arena *arena) {
  tv->verses = NULL;
  tv->count  = 0;
  tv->cap    = 0;
  tv->arena  = arena;
}

Verse* tv_alloc_array(Arena* arena, usize n) {
  if (n == 0) return NULL;
  // compute bytes carefully
  size_t bytes = n * sizeof(Verse);
  // overflow check
  if (bytes / sizeof(Verse) != n) return NULL;
  return (Verse*)arena_alloc(arena, bytes);
}


/* We're using this approach as we want ThreadVerses to be a contiguous block */
void push_verse(ThreadVerses* tv, Verse v) {
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

Verse parse_line(String8 line) {
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

void split_buffer_lines(String8 string, i32 num_threads, SliceArg* slices, ThreadVerses* outs) {
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
