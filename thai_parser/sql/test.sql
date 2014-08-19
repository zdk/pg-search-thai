CREATE EXTENSION thai_parser;
CREATE TEXT SEARCH CONFIGURATION testthaicfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION testthaicfg ADD MAPPING FOR a WITH simple;

select * from ts_parse('thai_parser', 'ภาษาไทยเป็นภาษาที่ง่ายที่สุดในโลก');
select to_tsvector('testthaicfg', 'ภาษาไทยเป็นภาษาที่ง่ายที่สุดในโลก');
SELECT to_tsquery('testthaicfg', 'ภาษาไทยเป็นภาษาที่ง่ายที่สุดในโลก');

select 'ERROR ts_parse for ภาษาไทยเป็นภาษาที่ง่ายที่สุดในโลก' where 
    (select count(*) from ts_parse('thai_parser', 'ภาษาไทยเป็นภาษาที่ง่ายที่สุดในโลก'))<>9;
