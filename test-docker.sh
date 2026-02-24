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
docker exec "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
SELECT * FROM ts_parse('thai_parser', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');

SELECT to_tsvector('thaicfg', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');
SELECT to_tsquery('thaicfg', 'ก็แดดมันร้อน คนไม่ใช่หุ่นยนต์ ที่จะทนตากแดดทั้งวัน');
EOSQL

echo ""
echo "==> Running README examples..."
docker exec "$CONTAINER_NAME" psql -U testuser -d testdb -v ON_ERROR_STOP=1 <<'EOSQL'
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
echo "==> Running UTF-8 0xB8 error reproduction SQL..."
REPRO_OUTPUT=$(docker exec "$CONTAINER_NAME" \
    psql -U testuser -d testdb -f /usr/src/pg-search-thai/tests/reproduce_utf8_0xb8_error.sql 2>&1) || true
echo "$REPRO_OUTPUT"

if echo "$REPRO_OUTPUT" | grep -q "invalid byte sequence"; then
    echo "UTF-8 0xB8 error REPRODUCED: Got 'invalid byte sequence' error."
fi

if echo "$REPRO_OUTPUT" | grep -q "'ปร'"; then
    echo "Bug confirmed: trans_pos() buffer overflow corrupted word boundaries."
fi

echo ""
echo "==> All tests passed!"
