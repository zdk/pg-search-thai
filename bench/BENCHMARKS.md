# pg-search-thai vs Elasticsearch — Thai full-text search benchmark

- Corpus: Thai Wikipedia, **100,000 documents**
- Both engines: single-node, single Docker host, no replicas
- Query iterations per engine: 5 × 50 queries

## Indexing

| Metric | pg-search-thai | Elasticsearch (thai analyzer) |
|---|---|---|
| Docs ingested | 100,000 | 100,000 |
| Bulk load (s) | 16.18 | 346.02 |
| tsvector populate (s) | 331.58 | n/a (analyzer runs at index time) |
| Index build (s) | 56.96 | included in load |
| Index size | 104.9 MB | 426.6 MB |
| Total table+index | 889.9 MB | 426.6 MB |

## Query latency

| Query kind | Engine | n | p50 | p95 | p99 | mean |
|---|---|---|---|---|---|---|
| and | pg-search-thai | 40 | 1.629 ms | 5.84 ms | — | 2.222 ms |
| and | ES thai | 40 | 6.653 ms | 9.8 ms | — | 7.001 ms |
| or | pg-search-thai | 35 | 2.605 ms | 3.226 ms | — | 2.161 ms |
| or | ES thai | 35 | 6.144 ms | 9.588 ms | — | 6.714 ms |
| single | pg-search-thai | 175 | 1.415 ms | 4.334 ms | 9.946 ms | 1.851 ms |
| single | ES thai | 175 | 5.656 ms | 18.891 ms | 27.288 ms | 7.521 ms |

## Per-query top-10 hit counts

Sanity check, not a recall measurement. Both engines were asked
for the top 10 matches; this just shows they returned similar
result-set sizes. A real recall test needs labelled relevance judgements.

| Query | PG hits | ES hits |
|---|---|---|
| `and:กรุงเทพ ประวัติศาสตร์` | 10 | 10 |
| `and:กีฬา โอลิมปิก` | 10 | 10 |
| `and:ประเทศไทย วัฒนธรรม` | 10 | 10 |
| `and:พุทธศาสนา วัด` | 10 | 10 |
| `and:มหาวิทยาลัย การศึกษา` | 10 | 10 |
| `and:วิทยาศาสตร์ เทคโนโลยี` | 10 | 10 |
| `and:อาหารไทย ภาคเหนือ` | 10 | 10 |
| `and:เศรษฐกิจ การค้า` | 10 | 10 |
| `or:พระมหากษัตริย์ ราชวงศ์` | 10 | 10 |
| `or:ภาคเหนือ ภาคใต้` | 10 | 10 |
| `or:ภาพยนตร์ ดนตรี` | 10 | 10 |
| `or:รัฐบาล รัฐสภา` | 10 | 10 |
| `or:ส้มตำ ต้มยำกุ้ง` | 10 | 10 |
| `or:เชียงใหม่ เชียงราย` | 10 | 10 |
| `or:โรงเรียน มหาวิทยาลัย` | 10 | 10 |
| `single:กรุงเทพมหานคร` | 10 | 10 |
| `single:การศึกษา` | 10 | 10 |
| `single:การเกษตร` | 10 | 10 |
| `single:การเมือง` | 10 | 10 |
| `single:กีฬา` | 10 | 10 |
| `single:ข้าวเหนียว` | 10 | 10 |
| `single:จังหวัดเชียงใหม่` | 10 | 10 |
| `single:ดนตรีไทย` | 10 | 10 |
| `single:ต้มยำกุ้ง` | 10 | 10 |
| `single:ท่องเที่ยว` | 10 | 10 |
| `single:ประวัติศาสตร์` | 10 | 10 |
| `single:ประเทศไทย` | 10 | 10 |
| `single:ปรัชญา` | 10 | 10 |
| `single:พระบาทสมเด็จ` | 10 | 10 |
| `single:พุทธศาสนา` | 10 | 10 |
| `single:ภาคใต้` | 10 | 10 |
| `single:ภาพยนตร์` | 10 | 10 |
| `single:ภาษาไทย` | 10 | 10 |
| `single:มหาวิทยาลัย` | 10 | 10 |
| `single:รถไฟฟ้า` | 10 | 10 |
| `single:รัฐธรรมนูญ` | 10 | 10 |
| `single:ราชอาณาจักร` | 10 | 10 |
| `single:วรรณกรรม` | 10 | 10 |
| `single:วัฒนธรรม` | 10 | 10 |
| `single:วิทยาศาสตร์` | 10 | 10 |
| `single:ศิลปะ` | 10 | 10 |
| `single:สงครามโลก` | 10 | 10 |
| `single:สถาปัตยกรรม` | 10 | 10 |
| `single:สิ่งแวดล้อม` | 10 | 10 |
| `single:ส้มตำ` | 10 | 10 |
| `single:อาหารไทย` | 10 | 10 |
| `single:อุตสาหกรรม` | 10 | 10 |
| `single:เทคโนโลยี` | 10 | 10 |
| `single:เศรษฐกิจ` | 10 | 10 |
| `single:โรงเรียน` | 10 | 10 |

## Caveats

- Single host, single node — production clusters look different.
- ES thai analyzer is ICU-based; pg-search-thai uses libthai.
  Word-boundary disagreements are expected and not bugs.
- Cold-cache numbers not reported here; both engines warmed once before measurement.
- ES `refresh_interval` was set to `1s` after bulk load — write→searchable lag
  was therefore ~1s on the ES side. Postgres is transactional (0ms).
