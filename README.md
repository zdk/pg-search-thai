pg-search-thai
============================

pg-search-thai is full text search _PostgreSQL_ extension for Thai Language.

Its main purpose is:

To enable PostgreSQL Full Text Search in Thai language (Due to Thai Language does not use spaces to separate words)

## Performance

Benchmarked against **Elasticsearch 8.15 with the built-in `thai` analyzer** on
100,000 Thai Wikipedia articles, single Docker host, warm cache:

![Query latency by kind — pg-search-thai vs Elasticsearch](bench/latency.png)

| Metric | pg-search-thai | ES (thai analyzer) |
|---|---|---|
| Single-term query (p50 / p95) | **1.4 ms / 4.3 ms** | 5.7 ms / 18.9 ms |
| Boolean AND (p50 / p95) | **1.6 ms / 5.8 ms** | 6.7 ms / 9.8 ms |
| Boolean OR (p50 / p95) | **2.6 ms / 3.2 ms** | 6.1 ms / 9.6 ms |
| Index size | **105 MB** | 427 MB |
| Write → searchable lag | 0 ms (transactional) | ~1 s (refresh interval) |

Full reproducible harness, methodology, and caveats: see [`bench/BENCHMARKS.md`](bench/BENCHMARKS.md)
and [`bench/README.md`](bench/README.md).

## Installation

### Quick start with Docker

Spins up Postgres 18 with `thai_parser` pre-built and the extension auto-created:

    git clone https://github.com/zdk/pg-search-thai.git
    cd pg-search-thai
    docker compose up -d

Connect with `psql` (password: `testpass`):

    psql -h localhost -U testuser -d testdb \
      -c "SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้ง');"

### From source

Install `libthai` and the Postgres build headers via your package manager:

    # Debian / Ubuntu
    sudo apt install -y build-essential libthai-dev postgresql-server-dev-all

    # macOS (Homebrew)
    brew install libthai libdatrie

Then build and install the extension:

    git clone https://github.com/zdk/pg-search-thai.git
    cd pg-search-thai
    make
    sudo make install

To install only the parser (without the bundled hunspell dictionary files):

    cd thai_parser && make && sudo make install

## Usage

Enable the extension (UTF-8 databases only):

    CREATE EXTENSION thai_parser;

This installs the `thai_parser` parser and a text search configuration named `thaicfg`.

### Quick tour

Four primitives: `ts_parse` (debug), `to_tsvector` (index), `to_tsquery` (query), `@@` (match).

**1. Tokenize.** libthai finds word boundaries even without spaces:

```
SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้ง Thai shrimp soup');
 tokid | token
-------+--------
    97 | ต้มยำ
    97 | กุ้ง
    99 |
    98 | Thai
    99 |
    98 | shrimp
(...)
-- tokid 97 = Thai word, 98 = ASCII word, 99 = space
```

**2. Build a `tsvector`.** `thaicfg` only indexes Thai tokens — English passes through unindexed unless you add a mapping for `asciiword`:

```
SELECT to_tsvector('thaicfg', 'ต้มยำกุ้ง');
   to_tsvector
-----------------
 'กุ้ง':2 'ต้มยำ':1
```

**3. Match.** `@@` returns boolean:

```
SELECT to_tsvector('thaicfg', 'ส้มตำกับข้าวเหนียว')
    @@ to_tsquery('thaicfg', 'ส้มตำ');
 ?column?
----------
 t
```

**4. Boolean operators** — `&` and, `|` or, `!` not, `()` group. Thai words written together still match:

```
-- "ข้าวเหนียวส้มตำ" with no space between the two words:
SELECT to_tsvector('thaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง')
    @@ to_tsquery('thaicfg', 'ข้าวเหนียว & ส้มตำ');
 ?column?
----------
 t
```

### Searching a real table

The shape you'll use in an app — store text, keep a `tsvector` in sync, GIN-index it, query through the index:

```sql
CREATE TABLE articles (
    id     SERIAL PRIMARY KEY,
    title  TEXT NOT NULL,
    body   TEXT NOT NULL,
    search tsvector GENERATED ALWAYS AS
        (to_tsvector('thaicfg', title || ' ' || body)) STORED
);
CREATE INDEX articles_search_idx ON articles USING GIN (search);

INSERT INTO articles (title, body) VALUES
    ('ส้มตำไทย',  'ส้มตำเป็นอาหารพื้นบ้านของภาคอีสาน'),
    ('ต้มยำกุ้ง', 'ต้มยำกุ้งเป็นซุปรสจัดที่มีชื่อเสียงระดับโลก'),
    ('ผัดไทย',   'ผัดไทยเป็นอาหารเส้นที่นิยมไปทั่วประเทศ');

SELECT id, title, ts_rank(search, q) AS rank
FROM   articles, to_tsquery('thaicfg', 'ส้มตำ') q
WHERE  search @@ q
ORDER  BY rank DESC;
 id |  title   |  rank
----+----------+--------
  1 | ส้มตำไทย |  0.076
```

### Advanced: hunspell dictionary

For richer normalization, drop Thai hunspell files into `$(pg_config --sharedir)/tsearch_data/`:

    CREATE TEXT SEARCH DICTIONARY thai_hunspell (
        TEMPLATE  = ispell,
        DictFile  = th_TH,
        AffFile   = th_TH,
        StopWords = english
    );
    ALTER TEXT SEARCH CONFIGURATION thaicfg
        ADD MAPPING FOR a WITH simple, thai_hunspell;
    SELECT ts_lexize('thai_hunspell', 'ทดสอบ');

Run `\dFd` in `psql` to confirm the dictionary is installed.

## Bugs Report and Contributing

GitHub issue tracker and pull requests are welcome.

_pg-search-thai_ is released under the GNU General Public License (GPLv2).
Refer to License [FAQ](http://www.gnu.org/licenses/old-licenses/gpl-2.0-faq.html) for more information.
