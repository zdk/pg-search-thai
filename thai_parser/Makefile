MODULE_big = thai_parser
OBJS = src/thai_parser.o src/converter.o

EXTENSION = thai_parser
DATA = sql/thai_parser--1.0.sql sql/thai_parser--unpackaged--1.0.sql

REGRESS = thai_parser

LIB = /usr/local/lib/
PG_CPPFLAGS = -I$(THAI_HOME)/include
SHLIB_LINK = -liconv -lthai -L$(LIB)

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


test:
	gcc src/converter.c tests/test_thai_parser.c -lthai -liconv -g -o tests/test_tp
	./tests/test_all.sh
