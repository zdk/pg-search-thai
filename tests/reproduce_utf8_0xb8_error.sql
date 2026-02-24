-- Reproduction tests for UTF-8 0xB8 error on Thai text tokenization

\echo 'Test A: Long Thai string (triggers trans_pos buffer overflow)'
SELECT to_tsvector('thaicfg',
    'ก็แดดมันร้อนคนไม่ใช่หุ่นยนต์ที่จะทนตากแดดทั้งวันก็อยากจะซื้อหุ่นยนต์สักตัวเอาไว้ใช้ทำครัวใช้กรีดยางตัดอ้อยขุดมันแต่กลัวหน้าหนาวกลัวหุ่นยนต์หนุ่มสาวไม่ชอบหนาวแอบก่อไฟผิง'
);

\echo 'Test B: Multiple rows with Thai text'
CREATE TEMPORARY TABLE thai_test_data (id serial, content text);
INSERT INTO thai_test_data (content) VALUES
    ('ทดสอบภาษาไทย'),
    ('สวัสดีครับ'),
    ('ภาษาไทยสำหรับการค้นหา'),
    ('ระบบค้นหาข้อความ'),
    ('ฐานข้อมูลPostgreSQL'),
    ('การทดสอบระบบ'),
    ('ข้อความภาษาไทย'),
    ('โปรแกรมคอมพิวเตอร์'),
    ('วิทยาศาสตร์และเทคโนโลยี'),
    ('มหาวิทยาลัยแห่งชาติ');
SELECT id, to_tsvector('thaicfg', content) FROM thai_test_data;

\echo 'Test C: Repeated calls stress test'
DO $$
DECLARE
    i int;
    v tsvector;
BEGIN
    FOR i IN 1..100 LOOP
        v := to_tsvector('thaicfg', 'ทดสอบการทำงานซ้ำหลายรอบ');
    END LOOP;
    RAISE NOTICE 'Completed 100 iterations without error';
END $$;

\echo 'All tests completed.'
