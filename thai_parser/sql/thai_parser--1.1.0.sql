CREATE FUNCTION thai_parser_start(internal, int4)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION thai_parser_get_token(internal, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION thai_parser_end(internal)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE FUNCTION thai_parser_lextype(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

CREATE TEXT SEARCH PARSER thai_parser (
    START    = thai_parser_start,
    GETTOKEN = thai_parser_get_token,
    END      = thai_parser_end,
    HEADLINE = pg_catalog.prsd_headline,
    LEXTYPES = thai_parser_lextype
);

CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;

-- Since 1.1.0 thaicfg also indexes English words (token type b). Before that it
-- mapped only Thai words, so any English in a text -- street names, brands,
-- CCTV, BTS -- was parsed and then silently dropped.
--
-- It has to be HERE, in the extension script, to survive pg_dump. thaicfg is an
-- extension member, and pg_dump never dumps changes made to extension members:
-- a dump says only `CREATE EXTENSION thai_parser`, so a mapping added
-- afterwards by the user disappears on restore without any error. Anything in
-- this script is recreated by that CREATE EXTENSION.
--
-- `simple` lowercases and nothing else, which suits proper nouns and
-- abbreviations better than `english` stemming.
ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR b WITH simple;
