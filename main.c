#include "core.h"
#include "bible.h"
#include "csv.h"
#include "util.h"

i32 main(i32 argc, const char* argv[]) {
  Arena arena = {0};
  arena_init(&arena, MB(8));
  String8 kjv_csv_contents = read_file(&arena, "data/kjv.csv", KB(2));
  CSVDocument kjv_csv = parse_csv(&arena, kjv_csv_contents);
  String8 kjv_txt = read_file(&arena, "data/kjv.txt", MB(5));

  u8 nthreads = N_THREADS;
  ThreadVerses* tvs = push_array(&arena, ThreadVerses, nthreads);
  Arena* thread_arenas = push_array(&arena, Arena, nthreads);

  for (i32 t = 0; t < nthreads; t++) {
    arena_init(&thread_arenas[t], KB(64));
    threadverses_init(&tvs[t], &thread_arenas[t]);
  }
  parse_bible_file_mt(&arena, kjv_txt, nthreads, tvs);

  u64 total_count = 0;
  for (i32 t = 0; t < nthreads; t++) {
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

  for (i32 t = 0; t < nthreads; t++) arena_free(&thread_arenas[t]);
  arena_free(&arena);
  return 0;
}

