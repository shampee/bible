#include "config.h"

Config config_parse(Arena* arena, OptList* options, usize noptions, String8List arglist) {
  Config result = {str8_zero(), str8_zero(), str8_zero(), N_CORES*2, LOOKUP};
  Temp scratch = scratch_begin(arena);

  ParsedArg* parsed_args = push_array(scratch.arena, ParsedArg, noptions);
  OptParseResult parsed_options = parse_options(options, noptions, parsed_args, arglist);

  for (u64 i = 0; i < parsed_options.parsed_count; i++) {
    ParsedArg parsed_arg = parsed_args[i];
    if (str8_match(parsed_arg.option, str8_lit("help"), 0)) {
      result.command = HELP;
      break;
    }
    if (str8_match(parsed_arg.option, str8_lit("bible"), 0)) {
      result.bible_path = str8_copy(arena, parsed_arg.option_arg);
    }
    if (str8_match(parsed_arg.option, str8_lit("csv"), 0)) {
      result.csv_path = str8_copy(arena, parsed_arg.option_arg);
    }
    if (str8_match(parsed_arg.option, str8_lit("threads"), 0)) {
      result.csv_path = str8_copy(arena, parsed_arg.option_arg);
    }
  }
  scratch_end(&scratch);

  StringJoin join = {str8_zero(), str8_lit(" "), str8_zero()};
  result.term = str8_list_join(arena, &parsed_options.leftovers, &join);

  return result;
}
