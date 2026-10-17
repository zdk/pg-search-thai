-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION thai_parser UPDATE TO '1.1.0'" to load this file. \quit

-- 1.1.0: thaicfg indexes English words (token type b) as well as Thai. See
-- thai_parser--1.1.0.sql for why this belongs in the extension script.
--
-- Guarded: a 1.0.0 database that wanted English had to add this mapping by
-- hand, and a second ADD MAPPING fails with a unique_violation. 98 is this
-- parser's token id for b.
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
          FROM pg_ts_config_map m
          JOIN pg_ts_config c ON c.oid = m.mapcfg
         WHERE c.oid = 'thaicfg'::regconfig AND m.maptokentype = 98
    ) THEN
        ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR b WITH simple;
    END IF;
END $$;

-- Stored tsvectors are NOT recomputed by this update: rows indexed under 1.0.0
-- still lack their English words until they are rebuilt. See README.md.
