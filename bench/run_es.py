"""Benchmark Elasticsearch with the bundled `thai` analyzer.

Mirrors run_pg.py: load → index → query, same query set, same iterations,
so report.py can put both side-by-side.
"""

import argparse
import json
import statistics
import time
from pathlib import Path

from elasticsearch import Elasticsearch, helpers
from tqdm import tqdm

import queryset

ES_URL = "http://localhost:9200"
INDEX = "documents"


def connect() -> Elasticsearch:
    return Elasticsearch(ES_URL, request_timeout=120)


def reset_index(es: Elasticsearch) -> None:
    if es.indices.exists(index=INDEX):
        es.indices.delete(index=INDEX)
    es.indices.create(
        index=INDEX,
        settings={
            # single-node, no replicas — fair fight against single PG
            "number_of_shards": 1,
            "number_of_replicas": 0,
            # disable refresh during bulk load; we'll force-merge after
            "refresh_interval": "-1",
        },
        mappings={
            "properties": {
                "id": {"type": "long"},
                "title": {"type": "text", "analyzer": "thai"},
                "body": {"type": "text", "analyzer": "thai"},
            }
        },
    )


def actions(corpus: Path):
    with corpus.open(encoding="utf-8") as f:
        for line in f:
            row = json.loads(line)
            yield {
                "_index": INDEX,
                "_id": row["id"],
                "_source": {
                    "id": row["id"],
                    "title": row["title"],
                    "body": row["text"],
                },
            }


def bulk_load(es: Elasticsearch, corpus: Path) -> tuple[int, float]:
    t0 = time.perf_counter()
    n = 0
    for ok, _ in helpers.streaming_bulk(
        es, actions(corpus),
        chunk_size=1000, request_timeout=120, raise_on_error=True,
    ):
        if ok:
            n += 1
        if n % 1000 == 0:
            tqdm.write(f"  indexed {n}")
    # Re-enable refresh and force a merge so query latency reflects a settled index.
    es.indices.put_settings(index=INDEX, settings={"refresh_interval": "1s"})
    es.indices.refresh(index=INDEX)
    es.indices.forcemerge(index=INDEX, max_num_segments=1, wait_for_completion=True)
    return n, time.perf_counter() - t0


def index_size(es: Elasticsearch) -> int:
    stats = es.indices.stats(index=INDEX, metric="store")
    return stats["_all"]["primaries"]["store"]["size_in_bytes"]


def to_es_query(q: queryset.Query) -> dict:
    if q.kind == "single":
        return {"multi_match": {"query": q.terms[0], "fields": ["title", "body"]}}
    op = "and" if q.kind == "and" else "or"
    return {
        "bool": {
            ("must" if op == "and" else "should"): [
                {"multi_match": {"query": t, "fields": ["title", "body"]}}
                for t in q.terms
            ],
            **({"minimum_should_match": 1} if op == "or" else {}),
        }
    }


def run_queries(
    es: Elasticsearch, queries: list[queryset.Query], iterations: int
) -> dict:
    by_kind: dict[str, list[float]] = {}
    hits: dict[str, int] = {}
    for q in tqdm(list(queryset.iter_repeated(queries, iterations)), desc="query", unit="q"):
        body = {"query": to_es_query(q), "size": 10, "_source": False}
        t0 = time.perf_counter()
        resp = es.search(index=INDEX, body=body)
        dt = (time.perf_counter() - t0) * 1000
        by_kind.setdefault(q.kind, []).append(dt)
        hits[q.label] = max(hits.get(q.label, 0), len(resp["hits"]["hits"]))
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
    ap.add_argument("--iterations", type=int, default=5)
    args = ap.parse_args()

    queries = queryset.load(args.queries)
    print(f"loaded {len(queries)} queries")

    es = connect()
    print("reset index")
    reset_index(es)

    print("bulk load")
    n, load_secs = bulk_load(es, Path(args.corpus))

    idx_bytes = index_size(es)

    print("warm + run queries")
    run_queries(es, queries, iterations=1)
    results = run_queries(es, queries, iterations=args.iterations)

    out = {
        "engine": "elasticsearch-thai-analyzer",
        "docs": n,
        "load_secs": round(load_secs, 2),
        "index_bytes": idx_bytes,
        "iterations": args.iterations,
        "queries": results,
    }
    Path(args.output).write_text(json.dumps(out, indent=2, ensure_ascii=False))
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
