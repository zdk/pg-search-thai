CREATE EXTENSION thai_parser;
DROP TEXT SEARCH CONFIGURATION testthaicfg;
CREATE TEXT SEARCH CONFIGURATION testthaicfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION testthaicfg ADD MAPPING FOR a WITH simple;

select * from ts_parse('thai_parser', 'สวัสดีครับทดสอบภาษาไทย');
select to_tsvector('testthaicfg', 'สวัสดีครับทดสอบภาษาไทย');
SELECT to_tsquery('testthaicfg', 'สวัสดีครับทดสอบภาษาไทย');

select 'ERROR ts_parse for สวัสดีครับทดสอบภาษาไทย' where
    (select count(*) from ts_parse('thai_parser', 'สวัสดีครับทดสอบภาษาไทย'))<>9;
