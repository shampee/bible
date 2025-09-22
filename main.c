#include "core.h"
#include "bible.h"
#include "config.h"
#include "csv.h"
#include "search.h"
#include "util.h"

i32 print_help(Arena* arena, OptList* options, u64 noptions);

i32 main(i32 argc, const char* argv[]) {
  Arena arena = {0};
  arena_init(&arena, MB(8));

  OptList options[] = {
    opt_list("--help",   "-h", OptTypeHasArgFalse,
            "display help"),
    opt_list("--bible",  "-b", OptTypeHasArgTrue,
            "path to bible text"),
    opt_list("--csv",    NULL, OptTypeHasArgTrue,
            "path to corresponding csv file"),
    opt_list("--threads", "-t", OptTypeHasArgTrue,
            "how many threads to use (default is 2 * number of cores)"),
  };
  usize noptions = ArrayCount(options);

  String8List arglist = {0};
  for (u64 i = 1; i < argc; i++) {
    str8_list_push(&arena, &arglist, str8((u8*)argv[i], strlen(argv[i])));
  }

  if (!arglist.first || arglist.node_count == 0) {
    return print_help(&arena, options, noptions);
  }

  Config config = config_parse(&arena, options, noptions, arglist);
  if (config.command == HELP) {
    return print_help(&arena, options, noptions);
  }

  u8* start = (u8*)argv[0];
  u8* end = start+strlen(argv[0])-5; // "bible" is 5 characters
  String8 base_path = str8_range(start, end);
  String8 data_path = str8_cat(&arena, base_path, str8_lit("data/"));
  String8 bible_path =
    config.bible_path.size > 0
    ? config.bible_path
    : str8_cat(&arena, data_path, str8_lit("kjv.txt"));
  String8 csv_path =
    config.csv_path.size > 0
    ? config.csv_path
    : str8_cat(&arena, data_path, str8_lit("kjv.csv"));

  /* String8 kjv_csv_contents = read_file(&arena, "data/kjv.csv", KB(2)); */
  /* CSVDocument kjv_csv = parse_csv(&arena, kjv_csv_contents); */
  String8 bible_txt = read_file(&arena, (char*)bible_path.str, MB(5));

  u8 nthreads = config.nthreads;
  ThreadVerses* tvs = push_array(&arena, ThreadVerses, nthreads);
  Arena* thread_arenas = push_array(&arena, Arena, nthreads);

  for (i32 t = 0; t < nthreads; t++) {
    arena_init(&thread_arenas[t], MB(1));
    threadverses_init(&thread_arenas[t], &tvs[t]);
  }
  parse_bible_file_mt(&arena, bible_txt, nthreads, tvs);

  String8List keys = {0};
  HashTable ht;
  hashtable_init(&arena, &ht, 1024);

  // Insert the whole bible into a hashtable
  // with keys in the form of: bible:book:chapter:verse
  // Each entry is a structure of 5 String8
  // representing the same information as the key plus the actual text.
  for (i32 t = 0; t < nthreads; t++) {
    ThreadVerses tv = tvs[t];
    for (i32 i = 0; i < tv.count; i++) {
      // Copying, because otherwise we insert something pointing at the same thing over and over
      Verse* copy = push_one(&arena, Verse);
      MemoryCopy(copy, &tv.verses[i], sizeof(Verse));
      String8 key = str8f(&arena,
                          "%.*s:%.*s:%.*s:%.*s",
                          str8_varg(copy->bible),
                          str8_varg(copy->book),
                          str8_varg(copy->chapter),
                          str8_varg(copy->verse));
      str8_list_push(&arena, &keys, key);
      hashtable_insert(&arena, &ht, key, (void*)copy);
    }
  }

  DEBUG("looking for keys using term: %s", config.term.str);
  SearchResult search = search_keys(&arena, config.term, keys);
  DEBUG("using key: %s", search.closest.str);
  /* printf(SGR_1FMT(1, "%.*s\n", str8_varg(search.closest))); */
  Verse* lookup_verse = (Verse*)hashtable_lookup(&ht, search.closest);

  LOG("%.*s", str8_varg(lookup_verse->text));

  for (i32 t = 0; t < nthreads; t++) arena_free(&thread_arenas[t]);
  arena_free(&arena);
  return 0;
}


i32 print_help(Arena* arena, OptList* options, u64 noptions) {
  LOG("bible v0.4.0");
  LOG("Usage: bible [TERM]...");
  LOG("Find bible verses based on search term");
  print_options(options, noptions, 2);
  arena_free(arena);
  return 1;
}
