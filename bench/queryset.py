"""Shared query-set parser used by both run_pg.py and run_es.py."""

from dataclasses import dataclass
from typing import Iterator


@dataclass
class Query:
    kind: str          # single | and | or
    terms: list[str]   # 1 term for single, 2 for and/or

    @property
    def label(self) -> str:
        return f"{self.kind}:{' '.join(self.terms)}"


def load(path: str) -> list[Query]:
    out: list[Query] = []
    with open(path, encoding="utf-8") as f:
        for raw in f:
            line = raw.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            kind, terms = parts[0], parts[1:]
            if kind == "single" and len(terms) != 1:
                raise ValueError(f"single query needs 1 term: {line}")
            if kind in ("and", "or") and len(terms) != 2:
                raise ValueError(f"{kind} query needs 2 terms: {line}")
            out.append(Query(kind=kind, terms=terms))
    return out


def iter_repeated(queries: list[Query], iterations: int) -> Iterator[Query]:
    for _ in range(iterations):
        for q in queries:
            yield q
