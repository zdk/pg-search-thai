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
