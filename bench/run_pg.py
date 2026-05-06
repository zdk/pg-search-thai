"""Benchmark pg-search-thai end-to-end: load → index → query.

Reports indexing throughput, GIN index size, and query latency percentiles
to a JSON file consumed by report.py.
"""

import argparse
import json
import statistics
import time
from pathlib import Path

import psycopg
from psycopg import sql
from tqdm import tqdm

import queryset

DSN = "host=localhost port=5433 dbname=bench user=bench password=bench"
TABLE = "documents"
CONFIG = "thaicfg"  # text-search config registered by the extension


def connect() -> psycopg.Connection:
    return psycopg.connect(DSN, autocommit=False)


def reset_schema(cur: psycopg.Cursor) -> None:
    cur.execute(f"DROP TABLE IF EXISTS {TABLE}")
    cur.execute(f"""
        CREATE TABLE {TABLE} (
            id     BIGINT PRIMARY KEY,
            title  TEXT,
            body   TEXT,
            tsv    tsvector
        )
    """)


def bulk_load(cur: psycopg.Cursor, corpus: Path) -> tuple[int, float]:
    """COPY the corpus in. Returns (rows, seconds)."""
    t0 = time.perf_counter()
    n = 0
    # COPY ... FROM STDIN with text format. Use \t separator and escape newlines/tabs.
    copy_sql = f"COPY {TABLE} (id, title, body) FROM STDIN"
    with cur.copy(copy_sql) as cp, corpus.open(encoding="utf-8") as f:
        for line in tqdm(f, desc="copy", unit="doc"):
            row = json.loads(line)
            title = (row["title"] or "").replace("\\", "\\\\").replace("\t", " ").replace("\n", " ")
            body = (row["text"] or "").replace("\\", "\\\\").replace("\t", " ").replace("\n", " ")
            cp.write_row((row["id"], title, body))
            n += 1
    return n, time.perf_counter() - t0


def populate_tsv(cur: psycopg.Cursor) -> float:
    """Compute tsvector via thaicfg. Single UPDATE so we time the parser hot path."""
    t0 = time.perf_counter()
    cur.execute(
        f"UPDATE {TABLE} SET tsv = to_tsvector(%s, coalesce(title,'') || ' ' || coalesce(body,''))",
        (CONFIG,),
    )
    return time.perf_counter() - t0


def build_index(cur: psycopg.Cursor) -> float:
    t0 = time.perf_counter()
    cur.execute(f"CREATE INDEX {TABLE}_tsv_gin ON {TABLE} USING GIN (tsv)")
    cur.execute("ANALYZE " + TABLE)
    return time.perf_counter() - t0


def index_size(cur: psycopg.Cursor) -> int:
    cur.execute(f"SELECT pg_relation_size('{TABLE}_tsv_gin')")
    return cur.fetchone()[0]


def table_size(cur: psycopg.Cursor) -> int:
    cur.execute(f"SELECT pg_total_relation_size('{TABLE}')")
    return cur.fetchone()[0]


def to_tsquery(q: queryset.Query) -> str:
    op = "&" if q.kind == "and" else "|" if q.kind == "or" else None
    if op is None:
        return q.terms[0]
    return f"{q.terms[0]} {op} {q.terms[1]}"


def run_queries(
    cur: psycopg.Cursor, queries: list[queryset.Query], iterations: int
) -> dict:
    by_kind: dict[str, list[float]] = {}
    hits: dict[str, int] = {}
    for q in tqdm(list(queryset.iter_repeated(queries, iterations)), desc="query", unit="q"):
        tsq = to_tsquery(q)
        # LIMIT 10 to reflect "top results" UX, not full scan
        sqltxt = (
            f"SELECT id FROM {TABLE} "
            f"WHERE tsv @@ to_tsquery(%s, %s) LIMIT 10"
        )
        t0 = time.perf_counter()
        cur.execute(sqltxt, (CONFIG, tsq))
        rows = cur.fetchall()
        dt = (time.perf_counter() - t0) * 1000  # ms
        by_kind.setdefault(q.kind, []).append(dt)
        hits[q.label] = max(hits.get(q.label, 0), len(rows))
    return {
        "by_kind": {k: percentiles(v) for k, v in by_kind.items()},
        "hits": hits,
    }


def percentiles(samples: list[float]) -> dict:
    s = sorted(samples)
    return {
        "n": len(s),
        "p50_ms": round(statistics.median(s), 3),
        "p95_ms": round(s[int(len(s) * 0.95) - 1], 3),
        "p99_ms": round(s[int(len(s) * 0.99) - 1], 3) if len(s) >= 100 else None,
        "mean_ms": round(statistics.fmean(s), 3),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--corpus", required=True)
    ap.add_argument("--queries", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--iterations", type=int, default=5,
                    help="how many times to replay the full query set")
    args = ap.parse_args()

    queries = queryset.load(args.queries)
    print(f"loaded {len(queries)} queries")

    with connect() as conn:
        with conn.cursor() as cur:
            print("reset schema")
            reset_schema(cur)
            conn.commit()

            print("bulk load")
            n, copy_secs = bulk_load(cur, Path(args.corpus))
            conn.commit()

            print("populate tsvector")
            tsv_secs = populate_tsv(cur)
            conn.commit()

            print("build GIN index")
            idx_secs = build_index(cur)
            conn.commit()

            idx_bytes = index_size(cur)
            tbl_bytes = table_size(cur)

            print("warm + run queries")
            # warm cache once before measurement
            run_queries(cur, queries, iterations=1)
            results = run_queries(cur, queries, iterations=args.iterations)

    out = {
        "engine": "pg-search-thai",
        "docs": n,
        "copy_secs": round(copy_secs, 2),
        "tsvector_secs": round(tsv_secs, 2),
        "index_secs": round(idx_secs, 2),
        "index_bytes": idx_bytes,
        "table_bytes": tbl_bytes,
        "iterations": args.iterations,
        "queries": results,
    }
    Path(args.output).write_text(json.dumps(out, indent=2, ensure_ascii=False))
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
