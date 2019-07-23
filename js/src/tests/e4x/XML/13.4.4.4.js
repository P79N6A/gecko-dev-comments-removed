









































gTestfile = '13.4.4.4.js';

START("13.4.4.4 - XML attribute()");

TEST(1, true, XML.prototype.hasOwnProperty("attribute"));


emps =
<employees count="2">
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST_XML(2, 2, emps.attribute("count"));


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST_XML(3, 0, emps.employee.(age == "25").attribute("id"));


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

TEST_XML(4, 0, emps.employee.(name == "Jim").attribute("id"));

END();
