CREATE EXTENSION thai_parser;
DROP TEXT SEARCH CONFIGURATION thaicfg;
CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;

select * from ts_parse('thai_parser', 'สวัสดีครับทดสอบภาษาไทย');
select to_tsvector('thaicfg', 'สวัสดีครับทดสอบภาษาไทย');
SELECT to_tsquery('thaicfg', 'สวัสดีครับทดสอบภาษาไทย');

select 'ERROR ts_parse for สวัสดีครับทดสอบภาษาไทย' where
    (select count(*) from ts_parse('thai_parser', 'สวัสดีครับทดสอบภาษาไทย'))<>9;
