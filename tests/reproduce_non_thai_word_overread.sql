-- reproduce_non_thai_word_overread.sql
--
-- Reproduces the over-read in get_non_thai_word(): an English word or number at
-- the very END of a text is read past the end of the string, so it absorbs
-- whatever bytes happen to follow it in memory. The damaged word is what gets
-- stored in the tsvector, so a search for the real word misses that row.
--
-- Run it with psql against any database where the thai_parser extension is
-- installed:
--
--     psql -d <db> -f tests/reproduce_non_thai_word_overread.sql
--
-- Everything is created inside the schema thai_overread_demo, which is dropped
-- again at the end. Nothing else in the database is touched.
--
-- Expected result
--   buggy library : section 2 reports a large number of damaged rows (typically
--                   30-60%), and section 3 finds fewer rows than exist
--   fixed library : section 2 reports 0 damaged rows, and section 3's two
--                   counts are equal for every word

\set ON_ERROR_STOP on
\pset footer off

DROP SCHEMA IF EXISTS thai_overread_demo CASCADE;
CREATE SCHEMA thai_overread_demo;

-- The stock thaicfg maps only Thai words (token type a), and drops English
-- words (type b) instead of storing them. Since the bug damages English words,
-- use a private configuration that keeps them, as a real application that
-- searches English words has to.
CREATE TEXT SEARCH CONFIGURATION thai_overread_demo.cfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION thai_overread_demo.cfg
    ADD MAPPING FOR a, b WITH simple;

-- A table like a complaint log: a Thai description that often ends with an
-- English word or a number, plus a search column computed from it.
CREATE TABLE thai_overread_demo.ticket (
    id          int PRIMARY KEY,
    last_word   text NOT NULL,   -- the word we put at the end, for checking
    description text NOT NULL,
    search      tsvector GENERATED ALWAYS AS
                    (to_tsvector('thai_overread_demo.cfg', description)) STORED
);

-- 2,000 rows: a Thai sentence followed by one of eight English words or
-- numbers as the very last word.
INSERT INTO thai_overread_demo.ticket (id, last_word, description)
SELECT i,
       w.word,
       (ARRAY['ไฟส่องสว่างดับทั้งซอย',
              'ถนนเป็นหลุมใหญ่อันตรายมาก',
              'ท่อระบายน้ำอุดตันน้ำท่วมขัง',
              'กล้องวงจรปิดเสียมาหลายวันแล้ว'])[1 + i % 4]
       || ' ' || w.word
FROM generate_series(1, 2000) AS i
JOIN (VALUES (0, 'CCTV'), (1, 'BTS'), (2, 'Sukhumvit'), (3, 'Lotus'),
             (4, 'MRT'), (5, 'SmartLighting'), (6, '2568'), (7, '12'))
     AS w(k, word) ON w.k = i % 8;

-- ---------------------------------------------------------------------------
\echo
\echo '== 1. A few stored rows. The last lexeme should be exactly the last word:'
SELECT id, description, search
FROM thai_overread_demo.ticket
ORDER BY id
LIMIT 8;

-- ---------------------------------------------------------------------------
\echo
\echo '== 2. How many rows had their last word damaged when stored?'
-- lower(last_word) is the lexeme that should be in the tsvector. If it is not
-- there, the word was stored with extra bytes glued on.
SELECT count(*) FILTER (WHERE NOT search @@ lower(last_word)::tsquery) AS damaged_rows,
       count(*)                                                        AS total_rows
FROM thai_overread_demo.ticket;

\echo
\echo '   Examples of damaged rows (what was actually stored):'
SELECT id, last_word, search
FROM thai_overread_demo.ticket
WHERE NOT search @@ lower(last_word)::tsquery
ORDER BY id
LIMIT 10;

-- ---------------------------------------------------------------------------
\echo
\echo '== 3. What a user sees: search for each word, compare with the real count'
-- 'cctv'::tsquery is written as a literal on purpose. It does not go through
-- the parser, so the search term itself cannot be damaged by the bug.
SELECT last_word                                               AS word,
       count(*)                                                AS rows_really_containing_it,
       count(*) FILTER (WHERE search @@ lower(last_word)::tsquery) AS rows_found_by_search
FROM thai_overread_demo.ticket
GROUP BY last_word
ORDER BY last_word;

-- ---------------------------------------------------------------------------
\echo
\echo '== 4. Garbage words now in the index (words that were never in any text):'
SELECT word, ndoc AS rows
FROM ts_stat('SELECT search FROM thai_overread_demo.ticket')
WHERE word !~ '[ก-๙]'
  AND word NOT IN ('cctv', 'bts', 'sukhumvit', 'lotus', 'mrt',
                   'smartlighting', '2568', '12')
ORDER BY ndoc DESC, word
LIMIT 15;

-- ---------------------------------------------------------------------------
-- Clean up. Comment this out to keep the table and look around yourself.
DROP SCHEMA thai_overread_demo CASCADE;
