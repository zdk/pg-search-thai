CREATE EXTENSION thai_parser;

-- Unaligned output, so column widths cannot differ between psql versions.
\pset format unaligned
\pset tuples_only on

-- Token types. This call used to crash the server.
SELECT * FROM ts_token_type('thai_parser');

-- Thai run with several words
SELECT * FROM ts_parse('thai_parser', 'ข้าวเหนียวส้มตำไก่ย่าง');
-- Thai run that is one word, so libthai finds no break
SELECT * FROM ts_parse('thai_parser', 'กรุงเทพ');
-- Thai mixed with ASCII words and spaces
SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้ง Thai soup 2024');

-- Non-Thai chars must not join a Thai run.
-- Curly quotes
SELECT * FROM ts_parse('thai_parser', '“ต้มยำกุ้ง”อร่อย');
-- Emoji
SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้ง😀อร่อย');
-- Zero width space
SELECT * FROM ts_parse('thai_parser', U&'ต้มยำกุ้ง\200Bอร่อย');
-- No-break space
SELECT * FROM ts_parse('thai_parser', U&'ต้มยำกุ้ง\00A0อร่อย');
-- Accented Latin stays one word
SELECT * FROM ts_parse('thai_parser', 'ร้านcaféอร่อย');
-- Thai digits
SELECT * FROM ts_parse('thai_parser', 'ราคา๑๕๐บาท');
-- Chinese
SELECT * FROM ts_parse('thai_parser', 'กิน中文ภาษาไทย');
-- A control char must not cut the rest of the text
SELECT to_tsvector('thaicfg', E'ต้มยำ\x01กุ้ง อร่อย');

-- The parser must stop at the end of its text, not read the next column.
CREATE TEMP TABLE overread (a text, b text);
INSERT INTO overread VALUES ('abc', repeat('x', 15));
SELECT (ts_parse('thai_parser', a)).* FROM overread;

-- A long text in curly quotes is still indexed.
SELECT length(to_tsvector('thaicfg', '“' || repeat('ต้มยำกุ้ง', 200) || '”'));
-- A long Thai run with no spaces parses in linear time.
SELECT count(*) FROM ts_parse('thai_parser', repeat('ต้มยำกุ้ง', 20000));

-- Indexing and matching
SELECT to_tsvector('thaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง');
SELECT to_tsvector('thaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง') @@ to_tsquery('thaicfg', 'ข้าวเหนียว & ส้มตำ');
SELECT to_tsvector('thaicfg', 'ส้มตำไก่ย่าง') @@ to_tsquery('thaicfg', 'ข้าวเหนียว & ส้มตำ');
SELECT to_tsvector('thaicfg', '“ต้มยำกุ้ง”อร่อย') @@ to_tsquery('thaicfg', 'กุ้ง');
SELECT ts_headline('thaicfg', 'ส้มตำไทย ต้มยำกุ้ง', to_tsquery('thaicfg', 'ต้มยำ'));

-- Non-Thai words are indexed too; curly quotes are not part of them.
SELECT to_tsvector('thaicfg', 'ราคา๑๕๐บาท “CCTV” café');
