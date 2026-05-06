"""Render BENCHMARKS.md (+ charts) from results-pg.json + results-es.json."""

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # headless: no display required
import matplotlib.pyplot as plt
import numpy as np

# Brand-ish colors so pg vs ES is visually distinct.
PG_COLOR = "#336791"  # postgres blue
ES_COLOR = "#FEC514"  # elastic yellow


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


def _annotate(ax, bars, fmt: str) -> None:
    for b in bars:
        h = b.get_height()
        ax.text(b.get_x() + b.get_width() / 2, h, fmt.format(h),
                ha="center", va="bottom", fontsize=8)


def plot_latency(pg_kinds: dict, es_kinds: dict, output: Path) -> None:
    """Grouped bar chart: p50 and p95 latency per query kind, pg vs ES."""
    kinds = sorted(set(pg_kinds) | set(es_kinds))
    pg_p50 = [(pg_kinds.get(k) or {}).get("p50_ms") or 0 for k in kinds]
    es_p50 = [(es_kinds.get(k) or {}).get("p50_ms") or 0 for k in kinds]
    pg_p95 = [(pg_kinds.get(k) or {}).get("p95_ms") or 0 for k in kinds]
    es_p95 = [(es_kinds.get(k) or {}).get("p95_ms") or 0 for k in kinds]

    x = np.arange(len(kinds))
    width = 0.38
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

    for ax, pg_v, es_v, title in (
        (ax1, pg_p50, es_p50, "p50 latency (ms)"),
        (ax2, pg_p95, es_p95, "p95 latency (ms)"),
    ):
        b1 = ax.bar(x - width / 2, pg_v, width, label="pg-search-thai", color=PG_COLOR)
        b2 = ax.bar(x + width / 2, es_v, width, label="ES thai", color=ES_COLOR)
        ax.set_title(title)
        ax.set_xticks(x)
        ax.set_xticklabels(kinds)
        ax.set_ylabel("ms")
        ax.grid(axis="y", alpha=0.3)
        ax.legend()
        _annotate(ax, b1, "{:.2f}")
        _annotate(ax, b2, "{:.2f}")

    fig.suptitle("Query latency by kind — lower is better")
    fig.tight_layout()
    fig.savefig(output, dpi=120)
    plt.close(fig)


def plot_indexing(pg: dict, es: dict, output: Path) -> None:
    """Side-by-side bars: total index time, then on-disk index size."""
    engines = ["pg-search-thai", "ES thai"]
    colors = [PG_COLOR, ES_COLOR]

    pg_total = pg["copy_secs"] + pg["tsvector_secs"] + pg["index_secs"]
    es_total = es["load_secs"]
    times = [pg_total, es_total]
    sizes_mb = [pg["index_bytes"] / 1024**2, es["index_bytes"] / 1024**2]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

    bars1 = ax1.bar(engines, times, color=colors)
    ax1.set_title("Total indexing time (s) — lower is better")
    ax1.set_ylabel("seconds")
    ax1.grid(axis="y", alpha=0.3)
    _annotate(ax1, bars1, "{:.1f}s")

    bars2 = ax2.bar(engines, sizes_mb, color=colors)
    ax2.set_title("Index size (MB) — lower is better")
    ax2.set_ylabel("MB")
    ax2.grid(axis="y", alpha=0.3)
    _annotate(ax2, bars2, "{:.0f} MB")

    fig.tight_layout()
    fig.savefig(output, dpi=120)
    plt.close(fig)


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

    out_path = Path(args.output)
    out_dir = out_path.parent
    latency_png = out_dir / "latency.png"
    indexing_png = out_dir / "indexing.png"
    plot_latency(pg_kinds, es_kinds, latency_png)
    plot_indexing(pg, es, indexing_png)

    md = []
    md.append("# pg-search-thai vs Elasticsearch — Thai full-text search benchmark\n")
    md.append(f"- Corpus: Thai Wikipedia, **{pg['docs']:,} documents**")
    md.append("- Both engines: single-node, single Docker host, no replicas")
    md.append(f"- Query iterations per engine: {pg['iterations']} × {sum(v['n'] for v in pg_kinds.values()) // pg['iterations']} queries\n")

    md.append("## Indexing\n")
    md.append(f"![Indexing time and size]({indexing_png.name})\n")
    md.append("| Metric | pg-search-thai | Elasticsearch (thai analyzer) |")
    md.append("|---|---|---|")
    md.append(f"| Docs ingested | {pg['docs']:,} | {es['docs']:,} |")
    md.append(f"| Bulk load (s) | {pg['copy_secs']} | {es['load_secs']} |")
    md.append(f"| tsvector populate (s) | {pg['tsvector_secs']} | n/a (analyzer runs at index time) |")
    md.append(f"| Index build (s) | {pg['index_secs']} | included in load |")
    md.append(f"| Index size | {fmt_bytes(pg['index_bytes'])} | {fmt_bytes(es['index_bytes'])} |")
    md.append(f"| Total table+index | {fmt_bytes(pg['table_bytes'])} | {fmt_bytes(es['index_bytes'])} |\n")

    md.append("## Query latency\n")
    md.append(f"![Query latency by kind]({latency_png.name})\n")
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
