









































gTestfile = '13.4.4.7.js';

START("13.4.4.7 - XML childIndex()");

TEST(1, true, XML.prototype.hasOwnProperty("childIndex"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST(2, 0, emps.employee[0].childIndex());


TEST(3, 1, emps.employee.(age == "20").childIndex());

TEST(4, 1, emps.employee.(name == "Joe").childIndex());
   
END();
