# pg-search-thai benchmark harness

Compares `pg-search-thai` against **Elasticsearch with the bundled `thai`
analyzer** on the same Thai Wikipedia corpus, with the same query set.

The goal is not to declare a winner — it is to give Thai-language SaaS
engineers honest numbers for the buying decision: *can I stay inside
Postgres at my scale, or do I need a dedicated search engine?*

## What it measures

- Bulk load time
- Index build time + index size on disk
- Query latency (p50 / p95 / p99) split by query kind: `single`, `and`, `or`
- Top-10 hit-count parity (sanity check, not a recall study)

## Prerequisites

- Docker + Docker Compose
- Python 3.11+
- ~4 GB free RAM (ES gets 2 GB heap, PG gets 512 MB shared_buffers)
- ~8 GB free disk for the corpus + both indexes

## Reproduce

```bash
cd bench
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt

# 1. start pg + es
make up

# 2. fetch 100K Thai Wikipedia articles (streamed, ~5–10 min)
make corpus

# 3. run both benchmarks and emit BENCHMARKS.md
make bench

# tear down
make down
# wipe everything (volumes + corpus)
make nuke
```

`BENCHMARKS.md` is written next to the script.

## Files

| File | Purpose |
|---|---|
| `docker-compose.yml` | PG (this extension) + ES side-by-side |
| `Dockerfile.es` | ES 8.15 with `analysis-thai` plugin pre-installed |
| `load_corpus.py` | Streams `wikimedia/wikipedia` (`20231101.th`) into JSONL |
| `queries.txt` | ~50 Thai queries (single / AND / OR) |
| `queryset.py` | Shared parser used by both runners |
| `run_pg.py` | PG: COPY → `to_tsvector('thaicfg', ...)` → GIN → query |
| `run_es.py` | ES: bulk → `thai` analyzer index → query |
| `report.py` | Joins both result JSONs into `BENCHMARKS.md` |

## Fairness notes

- Both run on a single Docker host with one node and no replicas.
  This is a "fair on equal hardware" comparison, not a production-cluster one.
- ES has `refresh_interval: -1` during bulk load and is force-merged to
  one segment before queries — this is the most favourable ES setup.
- PG uses default planner settings except for the build-time tunables
  in `docker-compose.yml` (`maintenance_work_mem=512MB` etc.).
- Query cache is warmed once on each engine before the timed run.
- Latencies include client round-trip over `localhost`. They are
  representative of an app-server-on-same-host topology, not a remote one.

## Known limitations

- **Recall** is not measured rigorously — that would need a labelled
  relevance set. The hit-count table is a sanity check, no more.
- **Cold cache** numbers are not collected; both engines see warm caches.
- **Concurrency** is not exercised; one client, sequential queries.
- The ES thai analyzer is ICU-based, while `pg-search-thai` uses libthai.
  They will disagree on word boundaries on some inputs — expected, not a bug.

Tune iterations, query set, and corpus size to taste — `make corpus`
takes `--limit` via `python load_corpus.py --limit N`.
