#include "core.h"
#include "bible.h"
#include "csv.h"
#include "search.h"
#include "util.h"

i32 main(i32 argc, const char* argv[]) {
  Arena arena = {0};
  arena_init(&arena, MB(8));
  /* String8 kjv_csv_contents = read_file(&arena, "data/kjv.csv", KB(2)); */
  /* CSVDocument kjv_csv = parse_csv(&arena, kjv_csv_contents); */
  String8 kjv_txt = read_file(&arena, "data/kjv.txt", MB(5));

  u8 nthreads = N_THREADS;
  DEBUG("nthreads = %d", nthreads);
  ThreadVerses* tvs = push_array(&arena, ThreadVerses, nthreads);
  Arena* thread_arenas = push_array(&arena, Arena, nthreads);

  for (i32 t = 0; t < nthreads; t++) {
    arena_init(&thread_arenas[t], KB(64));
    threadverses_init(&thread_arenas[t], &tvs[t]);
  }
  parse_bible_file_mt(&arena, kjv_txt, nthreads, tvs);

  String8List keys = {0};
  HashTable ht;
  hashtable_init(&arena, &ht, 1024);

  u64 total_count = 0;
  for (i32 t = 0; t < nthreads; t++) {
    ThreadVerses tv = tvs[t];
    for (i32 i = 0; i < tv.count; i++) {
      Verse verse = tv.verses[i];
      String8 key = str8f(&arena,
                          "%.*s:%.*s:%.*s:%.*s",
                          str8_varg(verse.bible),
                          str8_varg(verse.book),
                          str8_varg(verse.chapter),
                          str8_varg(verse.verse));
      str8_list_push(&arena, &keys, key);
      hashtable_insert(&arena, &ht, key, (void*)&verse);
      /* DEBUG("%.*s", str8_varg(key)); */

      total_count++;
    }
  }
  arena_reset(&arena);

  /* Verse* verse = hashtable_lookup(&ht, str8_lit("kjv:Zep:2:14")); */
  String8 term =str8_lit("Zep11");
  SearchResult search = search_keys(&arena, term, keys);
  DEBUG("query count: %lu",search.query_count);
  DEBUG("with term %.*s, closest match: %.*s",
        str8_varg(term),
        str8_varg(search.closest));
  
  for (i32 t = 0; t < nthreads; t++) arena_free(&thread_arenas[t]);
  arena_free(&arena);
  return 0;
}

