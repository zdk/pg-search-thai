-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION thai_parser" to load this file. \quit

ALTER EXTENSION thai_parser ADD function thai_parser_start(internal,integer);
ALTER EXTENSION thai_parser ADD function thai_parser_get_token(internal,internal,internal);
ALTER EXTENSION thai_parser ADD function thai_parser_end(internal);
ALTER EXTENSION thai_parser ADD function thai_parser_lextype(internal);
ALTER EXTENSION thai_parser ADD text search parser thai_parser;
