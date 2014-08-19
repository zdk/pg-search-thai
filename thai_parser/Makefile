# contrib/thai_parser/Makefile

MODULE_big = thai_parser
OBJS = src/thai_parser.o

EXTENSION = thai_parser
DATA = sql/thai_parser--1.0.sql sql/thai_parser--unpackaged--1.0.sql

REGRESS = thai_parser

PG_CPPFLAGS = -I$(THAI_HOME)/include
SHLIB_LINK = -lthai -L$(THAI_HOME)/lib -Wl,-rpath=$(THAI_HOME)/src/.lib

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
