RESULT=`./tests/test_tp "ทดสอบการตัดคำภาษาไทยสำหรับ PostgreSQL Full Text Search"`

echo $RESULT
if [[ "$RESULT" == ">>ทดสอบการตัดคำภาษาไทยสำหรับ PostgreSQL Full Text Search<< ทดสอบ|การ|ตัด|คำ|ภาษา|ไทย|สำหรับ|no word |PostgreSQL| |Full| |Text| |Search|" ]]; then
    echo -e "\x1B[0;32m Passes.\x1B[0m"
else
    echo -e "\x1B[0;31m Failed!!\x1B[0m"
fi
