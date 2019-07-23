









































gTestfile = '13.5.4.7.js';

START("13.5.4.7 - XMLList contains()");

TEST(1, true, XMLList.prototype.hasOwnProperty("contains"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST(2, true, emps.employee.contains(<employee id="0"><name>Jim</name><age>25</age></employee>));
TEST(3, true, emps.employee.contains(<employee id="1"><name>Joe</name><age>20</age></employee>));
TEST(4, false, emps.employee.contains(<employee><name>Joe</name><age>20</age></employee>));

END();