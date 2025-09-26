#include "bible.h"

void threadverses_init(Arena* arena, ThreadVerses* tv) {
  tv->verses = NULL;
  tv->count  = 0;
  tv->cap    = 0;
  tv->arena  = arena;
}

Verse* tv_alloc_array(Arena* arena, usize n) {
  if (n == 0) return NULL;
  // compute bytes carefully
  usize bytes = n * sizeof(Verse);
  // overflow check
  if (bytes / sizeof(Verse) != n) return NULL;
  return (Verse*)arena_alloc(arena, bytes);
}


/* We're using this approach as we want verses to be a contiguous block */
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
void print_verse(Arena* arena, Verse* v, u64 max_line_length) {
  if (!v) return;

  LOG("%.*s %.*s:%.*s", str8_varg(v->book), str8_varg(v->chapter),
      str8_varg(v->verse));

  if (max_line_length == 0) {
    LOG("%.*s", str8_varg(v->text));
  } else {
    u64 cursor = 0;
    u64 i = 0;
    while (i < v->text.size) {
      u64 word_end = str8_find_needle(v->text, i, str8_lit(" "), 0);
      if (word_end == (u64)-1) {
        word_end = v->text.size;
      }

      u64 word_len = word_end - i;
      if (cursor > 0 && cursor + 1 + word_len > max_line_length) {
        printf("\n");
        cursor = 0;
      }

      if (cursor > 0) {
        printf(" ");
        cursor++;
      }

      fwrite(v->text.str + i, 1, word_len, stdout);
      cursor += word_len;

      i = word_end + 1;
    }
    printf("\n");
  }
}

Verse parse_line(String8 line) {
  u8* p = line.str;
  u8* end = line.str + line.size;
  u8* field_start = p;

  Verse v;

  while (p < end && *p != ':') p++;
  v.bible = str8_range(field_start, p);
  field_start = ++p;

  while (p < end && *p != ':') p++;
  v.book = str8_range(field_start, p);
  field_start = ++p;

  while (p < end && *p != ':') p++;
  v.chapter = str8_range(field_start, p);
  field_start = ++p;

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

  u8* p = arg->begin;
  while (p < arg->end) {
    u8* line_end = memchr(p, '\n', arg->end - p);
    if (!line_end) line_end = arg->end;

    String8 line = str8_range(p, line_end);
    Verse v = parse_line(line);
    push_verse(tv, v);

    p = line_end + 1;
  }

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
    pthread_join(threads[t], NULL);
  }
}
