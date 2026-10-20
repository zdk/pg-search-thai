#!/usr/bin/env bash
set -euo pipefail

CONTAINER_NAME="pg-search-thai-test"
IMAGE_NAME="pg-search-thai-test"

cleanup() {
    echo "==> Cleaning up..."
    docker rm -f "$CONTAINER_NAME" 2>/dev/null || true
}
trap cleanup EXIT

echo "==> Building Docker image..."
docker build -t "$IMAGE_NAME" .

echo "==> Starting PostgreSQL container..."
docker run -d \
    --name "$CONTAINER_NAME" \
    -e POSTGRES_DB=testdb \
    -e POSTGRES_USER=testuser \
    -e POSTGRES_PASSWORD=testpass \
    "$IMAGE_NAME"

echo "==> Waiting for PostgreSQL to be ready..."
for i in $(seq 1 30); do
    if docker exec "$CONTAINER_NAME" pg_isready -U testuser -d testdb >/dev/null 2>&1; then
        break
    fi
    if [ "$i" -eq 30 ]; then
        echo "ERROR: PostgreSQL did not become ready in time."
        docker logs "$CONTAINER_NAME"
        exit 1
    fi
    sleep 1
done
sleep 3

echo "==> Running C unit tests..."
C_TEST_OUTPUT=$(docker exec -w /usr/src/pg-search-thai/thai_parser "$CONTAINER_NAME" make test 2>&1)
echo "$C_TEST_OUTPUT"
if echo "$C_TEST_OUTPUT" | grep -q "Failed"; then
    echo "C unit tests FAILED!"
    exit 1
fi

echo ""
echo "==> Running SQL integration tests..."
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
SELECT * FROM ts_parse('thai_parser', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');

SELECT to_tsvector('thaicfg', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');
-- plainto_, not to_tsquery: to_tsquery needs & or | between words and rejects
-- a plain sentence with a syntax error. This went unnoticed because the block
-- never ran before docker exec got -i.
SELECT plainto_tsquery('thaicfg', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');
EOSQL

echo ""
echo "==> Running README examples..."
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
-- Example 1: ts_parse
SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');

-- Example 2: to_tsvector
SELECT to_tsvector('thaicfg', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');

-- Example 3: querying
DO $$
BEGIN
  IF NOT (SELECT to_tsvector('thaicfg', 'the land of somtum (ส้มตำ)') @@ to_tsquery('thaicfg','ส้มตำ')) THEN
    RAISE EXCEPTION 'FAILED: Example 3 should return true';
  END IF;
END $$;

-- Example 4: querying with | and &
DO $$
BEGIN
  IF (SELECT to_tsvector('thaicfg', 'ส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('thaicfg','ข้าวเหนียว&ส้มตำ')) THEN
    RAISE EXCEPTION 'FAILED: Example 4a should return false';
  END IF;
  IF NOT (SELECT to_tsvector('thaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('thaicfg','ข้าวเหนียว&ส้มตำ')) THEN
    RAISE EXCEPTION 'FAILED: Example 4b should return true';
  END IF;
END $$;
EOSQL

echo ""
echo "==> Running buffer overread test..."
docker exec -w /usr/src/pg-search-thai "$CONTAINER_NAME" \
    gcc -o tests/test_buffer_overread tests/test_buffer_overread.c -g 2>&1
OVERREAD_OUTPUT=$(docker exec -w /usr/src/pg-search-thai "$CONTAINER_NAME" \
    ./tests/test_buffer_overread 2>&1)
echo "$OVERREAD_OUTPUT"
if echo "$OVERREAD_OUTPUT" | grep -q "FAIL"; then
    echo "Buffer overread test had failures!"
    exit 1
fi

echo ""
echo "==> Running get_non_thai_word bounds test (English word at the end of the text)..."
docker exec -w /usr/src/pg-search-thai "$CONTAINER_NAME" \
    gcc -o tests/test_non_thai_word_bounds tests/test_non_thai_word_bounds.c \
        thai_parser/src/tokenizer.c thai_parser/src/converter.c -lthai 2>&1
BOUNDS_OUTPUT=$(docker exec -w /usr/src/pg-search-thai "$CONTAINER_NAME" \
    ./tests/test_non_thai_word_bounds 2>&1) || true
echo "$BOUNDS_OUTPUT"
if echo "$BOUNDS_OUTPUT" | grep -q "FAIL"; then
    echo "get_non_thai_word bounds test had failures!"
    exit 1
fi

echo ""
echo "==> Checking the same over-read end to end, inside PostgreSQL..."
# The C test above feeds the tokenizer a hand-made buffer. This checks the real
# path: PostgreSQL passes a text datum that is not NUL-terminated, so before the
# fix the last token of a text absorbed leftover bytes from earlier calls in
# more than half of these texts. ts_parse shows the raw tokens, so this does
# not depend on how thaicfg maps English words.
#
# Keep it ONE query over generate_series: the texts must be built next to each
# other in memory for the over-read to find letters to swallow. The same check
# written as a PL/pgSQL loop (one text per statement) passes even on the buggy
# code. MATERIALIZED makes PostgreSQL compute each token once, so the count and
# the example come from the same result.
#
# `docker exec -i` matters: without -i the heredoc never reaches psql and the
# block silently runs nothing.
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
DO $$
DECLARE bad int; example text;
BEGIN
  WITH s AS MATERIALIZED (
    SELECT 'END' || (i % 97) AS want,
           (SELECT token
              FROM ts_parse('thai_parser', 'กล้อง ' || repeat('x', 1 + i % 17) || ' END' || (i % 97))
                   WITH ORDINALITY AS p(tokid, token, n)
             ORDER BY n DESC LIMIT 1) AS got
      FROM generate_series(1, 5000) AS i)
  SELECT count(*) FILTER (WHERE got IS DISTINCT FROM want),
         min(want || ' -> ' || got) FILTER (WHERE got IS DISTINCT FROM want)
    INTO bad, example
    FROM s;
  IF bad > 0 THEN
    RAISE EXCEPTION 'FAILED: last token over-read in % of 5000 texts, e.g. %', bad, example;
  END IF;
  RAISE NOTICE 'OK: last token intact in all 5000 texts';
END $$;
EOSQL

echo ""
echo "==> Checking the token type list (ts_debug, unknown token type)..."
# thai_parser_lextype() used to put its end-of-list marker outside the array,
# so anything that walks the whole list read past it. ts_debug() and ADD
# MAPPING FOR an unknown type (e.g. asciiword, which the standard parser has)
# crashed the server with a segfault. If that regresses, psql loses the
# connection and this step fails.
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
DO $$
DECLARE n int;
BEGIN
  SELECT count(*) INTO n FROM ts_token_type('thai_parser');
  IF n <> 3 THEN
    RAISE EXCEPTION 'FAILED: expected 3 token types (a, b, c), got %', n;
  END IF;
  SELECT count(*) INTO n FROM ts_debug('thaicfg', 'ต้มยำกุ้ง Thai shrimp soup');
  IF n = 0 THEN
    RAISE EXCEPTION 'FAILED: ts_debug returned no rows';
  END IF;
  BEGIN
    ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR asciiword WITH simple;
    RAISE EXCEPTION 'FAILED: ADD MAPPING FOR asciiword should have been rejected';
  EXCEPTION WHEN invalid_parameter_value THEN
    NULL;   -- expected: token type "asciiword" does not exist
  END;
  RAISE NOTICE 'OK: token types listed, ts_debug works, unknown token type rejected';
END $$;
EOSQL

echo ""
echo "==> Running the README hunspell example..."
# Same statements as README "Advanced: hunspell dictionary". It used to fail:
# the files were installed as th_TH.* (PostgreSQL only opens lowercase names),
# and it did ADD MAPPING FOR a on thaicfg, which already maps a.
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
CREATE TEXT SEARCH DICTIONARY thai_hunspell (
    TEMPLATE  = ispell,
    DictFile  = th_th,
    AffFile   = th_th,
    StopWords = english
);
CREATE TEXT SEARCH CONFIGURATION thaicfg_hunspell (COPY = thaicfg);
ALTER TEXT SEARCH CONFIGURATION thaicfg_hunspell
    ALTER MAPPING FOR a WITH thai_hunspell, simple;
DO $$
BEGIN
  IF ts_lexize('thai_hunspell', 'ทดสอบ') IS DISTINCT FROM ARRAY['ทดสอบ'] THEN
    RAISE EXCEPTION 'FAILED: thai_hunspell did not recognize ทดสอบ';
  END IF;
  IF NOT (to_tsvector('thaicfg_hunspell', 'ทดสอบการตัดคำ') @@ to_tsquery('thaicfg_hunspell', 'ทดสอบ')) THEN
    RAISE EXCEPTION 'FAILED: thaicfg_hunspell does not index Thai words';
  END IF;
  RAISE NOTICE 'OK: README hunspell example works';
END $$;
DROP TEXT SEARCH CONFIGURATION thaicfg_hunspell;
DROP TEXT SEARCH DICTIONARY thai_hunspell;
EOSQL

echo ""
echo "==> Checking English indexing (1.1.0): fresh install, upgrade, dump/restore..."
docker exec -i "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
DO $$
BEGIN
  IF (SELECT extversion FROM pg_extension WHERE extname = 'thai_parser') <> '1.1.0' THEN
    RAISE EXCEPTION 'FAILED: expected thai_parser 1.1.0, got %',
      (SELECT extversion FROM pg_extension WHERE extname = 'thai_parser');
  END IF;
  IF NOT (to_tsvector('thaicfg', 'กล้องวงจรปิด CCTV BTS') @@ to_tsquery('thaicfg', 'cctv & bts')) THEN
    RAISE EXCEPTION 'FAILED: English words are not indexed on a fresh 1.1.0 install';
  END IF;
  -- The README's punctuation advice: '(CCTV)' indexes as '(cctv)' unless ASCII
  -- punctuation is stripped first, and stripping it must leave Thai intact.
  IF to_tsvector('thaicfg', 'กล้อง (CCTV)') @@ to_tsquery('thaicfg', 'cctv') THEN
    RAISE EXCEPTION 'FAILED: expected (CCTV) to index as (cctv), README says so';
  END IF;
  IF NOT (to_tsvector('thaicfg', regexp_replace('กล้องวงจรปิด (CCTV) เสีย', '[!-/:-@\[-`{-~]', ' ', 'g'))
          @@ to_tsquery('thaicfg', 'cctv & กล้อง & วงจร & เสีย')) THEN
    RAISE EXCEPTION 'FAILED: README punctuation-stripping example does not work';
  END IF;
END $$;

CREATE DATABASE upgrade_plain;
CREATE DATABASE upgrade_manual;
EOSQL
# Upgrade from 1.0.0, twice: a plain 1.0.0 database, and one that already
# added the English mapping by hand. The update must not fail on the second.
for db in upgrade_plain upgrade_manual; do
    extra=""
    [ "$db" = upgrade_manual ] && extra="ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR b WITH simple;"
    docker exec -i "$CONTAINER_NAME" psql -U testuser -d "$db" -v ON_ERROR_STOP=1 <<EOSQL
CREATE EXTENSION thai_parser VERSION '1.0.0';
$extra
ALTER EXTENSION thai_parser UPDATE TO '1.1.0';
DO \$\$
BEGIN
  IF NOT (to_tsvector('thaicfg', 'กล้อง CCTV') @@ to_tsquery('thaicfg', 'cctv')) THEN
    RAISE EXCEPTION 'FAILED: $db: English not indexed after ALTER EXTENSION UPDATE';
  END IF;
END \$\$;
EOSQL
done

# Dump and restore: pg_dump does not dump changes to extension members, so the
# English mapping survives only because the extension script creates it.
docker exec "$CONTAINER_NAME" sh -c '
  set -e
  pg_dump -U testuser -Fc testdb -f /tmp/roundtrip.dump
  createdb -U testuser restored
  pg_restore -U testuser -d restored /tmp/roundtrip.dump
'
docker exec -i "$CONTAINER_NAME" psql -U testuser -d restored -v ON_ERROR_STOP=1 <<'EOSQL'
DO $$
BEGIN
  IF NOT (to_tsvector('thaicfg', 'กล้อง CCTV') @@ to_tsquery('thaicfg', 'cctv')) THEN
    RAISE EXCEPTION 'FAILED: English mapping lost across pg_dump/pg_restore';
  END IF;
  RAISE NOTICE 'OK: English indexed on install, after upgrade and after restore';
END $$;
EOSQL

echo ""
echo "==> Running UTF-8 0xB8 error reproduction SQL..."
REPRO_OUTPUT=$(docker exec "$CONTAINER_NAME" \
    psql -U testuser -d testdb -f /usr/src/pg-search-thai/tests/reproduce_utf8_0xb8_error.sql 2>&1) || true
echo "$REPRO_OUTPUT"

REPRO_FAILED=0
if echo "$REPRO_OUTPUT" | grep -q "invalid byte sequence"; then
    echo "UTF-8 0xB8 error REPRODUCED: Got 'invalid byte sequence' error."
    REPRO_FAILED=1
fi

if echo "$REPRO_OUTPUT" | grep -q "'ปร'"; then
    echo "Bug confirmed: trans_pos() buffer overflow corrupted word boundaries."
    REPRO_FAILED=1
fi

if [ "$REPRO_FAILED" -eq 1 ]; then
    echo "FAILED: UTF-8 0xB8 regression test detected errors."
    exit 1
fi

echo ""
echo "==> All tests passed!"
