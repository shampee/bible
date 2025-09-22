CC = gcc
CFLAGS = -Wall -std=c11 -pedantic -Wno-missing-braces -Wmissing-field-initializers -pthread  -g
LFLAGS = -lm

BASE_SOURCES = base/base_core.c base/base_math.c base/base_strings.c base/base_args.c  base/base_log.c base/base_hashtable.c
BASE_HEADERS = base/base_arena.h base/base_core.h base/base_math.h base/base_strings.h base/base_args.h base/base_log.h base/base_log.h base/base_hashtable.h

SOURCE_FILES = $(BASE_SOURCES) main.c csv.c util.c bible.c search.c config.c
HEADER_FILES = $(BASE_HEADERS) core.h csv.h util.h bible.h search.h config.h


main.o: main.c
	$(CC) $(CFLAGS) $(SOURCE_FILES) $(HEADER_FILES) $(LFLAGS) -o bible


