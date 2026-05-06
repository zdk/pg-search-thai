.PHONY: up down corpus bench bench-pg bench-es report clean nuke

# bring up pg + es; --build picks up Dockerfile changes
up:
	docker compose up -d --build
	@echo "waiting for pg + es to become healthy..."
	@sleep 15

down:
	docker compose down

# fetch 100K Thai Wikipedia articles (streamed, no full dump download)
corpus:
	python load_corpus.py --limit 100000 --output corpus.jsonl

bench-pg:
	python run_pg.py --corpus corpus.jsonl --queries queries.txt --output results-pg.json

bench-es:
	python run_es.py --corpus corpus.jsonl --queries queries.txt --output results-es.json

bench: bench-pg bench-es report

report:
	python report.py --pg results-pg.json --es results-es.json --output BENCHMARKS.md
	@echo "wrote BENCHMARKS.md"

clean:
	rm -f results-*.json BENCHMARKS.md latency.png indexing.png

# wipe everything including corpus + volumes
nuke: clean
	rm -f corpus.jsonl
	docker compose down -v
