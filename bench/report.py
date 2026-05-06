"""Render BENCHMARKS.md from results-pg.json + results-es.json."""

import argparse
import json
from pathlib import Path


def fmt_bytes(n: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.1f} {unit}"
        n /= 1024
    return f"{n:.1f} TB"


def cell(d: dict | None, key: str) -> str:
    if d is None or key not in d or d[key] is None:
        return "—"
    return f"{d[key]} ms"


def overlap_table(pg: dict, es: dict) -> str:
    """Top-10 result-set agreement per query, as a sanity proxy for recall."""
    rows = ["| Query | PG hits | ES hits |", "|---|---|---|"]
    for label, n_pg in sorted(pg["queries"]["hits"].items()):
        n_es = es["queries"]["hits"].get(label, 0)
        rows.append(f"| `{label}` | {n_pg} | {n_es} |")
    return "\n".join(rows)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--pg", required=True)
    ap.add_argument("--es", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    pg = json.loads(Path(args.pg).read_text())
    es = json.loads(Path(args.es).read_text())

    pg_kinds = pg["queries"]["by_kind"]
    es_kinds = es["queries"]["by_kind"]
    kinds = sorted(set(pg_kinds) | set(es_kinds))

    md = []
    md.append("# pg-search-thai vs Elasticsearch — Thai full-text search benchmark\n")
    md.append(f"- Corpus: Thai Wikipedia, **{pg['docs']:,} documents**")
    md.append("- Both engines: single-node, single Docker host, no replicas")
    md.append(f"- Query iterations per engine: {pg['iterations']} × {sum(v['n'] for v in pg_kinds.values()) // pg['iterations']} queries\n")

    md.append("## Indexing\n")
    md.append("| Metric | pg-search-thai | Elasticsearch (thai analyzer) |")
    md.append("|---|---|---|")
    md.append(f"| Docs ingested | {pg['docs']:,} | {es['docs']:,} |")
    md.append(f"| Bulk load (s) | {pg['copy_secs']} | {es['load_secs']} |")
    md.append(f"| tsvector populate (s) | {pg['tsvector_secs']} | n/a (analyzer runs at index time) |")
    md.append(f"| Index build (s) | {pg['index_secs']} | included in load |")
    md.append(f"| Index size | {fmt_bytes(pg['index_bytes'])} | {fmt_bytes(es['index_bytes'])} |")
    md.append(f"| Total table+index | {fmt_bytes(pg['table_bytes'])} | {fmt_bytes(es['index_bytes'])} |\n")

    md.append("## Query latency\n")
    md.append("| Query kind | Engine | n | p50 | p95 | p99 | mean |")
    md.append("|---|---|---|---|---|---|---|")
    for k in kinds:
        for label, side in (("pg-search-thai", pg_kinds.get(k)), ("ES thai", es_kinds.get(k))):
            if side is None:
                continue
            md.append(
                f"| {k} | {label} | {side['n']} | "
                f"{cell(side,'p50_ms')} | {cell(side,'p95_ms')} | "
                f"{cell(side,'p99_ms')} | {cell(side,'mean_ms')} |"
            )
    md.append("")

    md.append("## Per-query top-10 hit counts\n")
    md.append("Sanity check, not a recall measurement. Both engines were asked")
    md.append("for the top 10 matches; this just shows they returned similar")
    md.append("result-set sizes. A real recall test needs labelled relevance judgements.\n")
    md.append(overlap_table(pg, es))
    md.append("")

    md.append("## Caveats\n")
    md.append("- Single host, single node — production clusters look different.")
    md.append("- ES thai analyzer is ICU-based; pg-search-thai uses libthai.")
    md.append("  Word-boundary disagreements are expected and not bugs.")
    md.append("- Cold-cache numbers not reported here; both engines warmed once before measurement.")
    md.append("- ES `refresh_interval` was set to `1s` after bulk load — write→searchable lag")
    md.append("  was therefore ~1s on the ES side. Postgres is transactional (0ms).\n")

    Path(args.output).write_text("\n".join(md))
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
