CC = gcc
CFLAGS = -Wall -std=c11 -pedantic -Wno-missing-braces -Wmissing-field-initializers -pthread -s -O2
LFLAGS = -lm
TARGET = bible

BASE_SOURCES = base/base_core.c base/base_math.c base/base_strings.c base/base_args.c  base/base_log.c base/base_hashtable.c
BASE_OBJS    = $(BASE_SOURCES:.c=.o)

SOURCE_FILES = main.c csv.c util.c bible.c search.c config.c
OBJFILES = $(SOURCE_FILES:.c=.o) $(BASE_OBJS)

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) $(LFLAGS) -o $@

clean:
	rm -rf $(OBJFILES) $(TARGET) *~
