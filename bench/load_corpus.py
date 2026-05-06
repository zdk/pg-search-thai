"""Stream Thai Wikipedia articles from HuggingFace and write JSONL.

Streaming avoids downloading the full ~1GB parquet shard set when we only need
the first 100K rows. Each line: {"id": int, "title": str, "text": str}.
"""

import argparse
import json
import sys

from datasets import load_dataset
from tqdm import tqdm


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=100_000)
    ap.add_argument("--output", default="corpus.jsonl")
    ap.add_argument("--config", default="20231101.th",
                    help="HF wikimedia/wikipedia config (snapshot.lang)")
    ap.add_argument("--min-chars", type=int, default=200,
                    help="skip stub articles below this length")
    args = ap.parse_args()

    ds = load_dataset(
        "wikimedia/wikipedia", args.config,
        split="train", streaming=True,
    )

    written = 0
    with open(args.output, "w", encoding="utf-8") as f:
        bar = tqdm(total=args.limit, unit="doc")
        for row in ds:
            text = row.get("text") or ""
            if len(text) < args.min_chars:
                continue
            out = {
                "id": written,  # dense local id; HF id is a string
                "title": row.get("title") or "",
                "text": text,
            }
            f.write(json.dumps(out, ensure_ascii=False))
            f.write("\n")
            written += 1
            bar.update(1)
            if written >= args.limit:
                break
        bar.close()

    print(f"wrote {written} docs to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
