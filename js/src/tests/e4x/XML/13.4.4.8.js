









































gTestfile = '13.4.4.8.js';

START("13.4.4.8 - XML children()");

TEST(1, true, XML.prototype.hasOwnProperty("children"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct = new XMLList();
correct += <employee id="0"><name>Jim</name><age>25</age></employee>;
correct += <employee id="1"><name>Joe</name><age>20</age></employee>;

TEST(2, "xml", typeof(emps.children()));
TEST(3, correct, emps.children());


correct = new XMLList();
correct += <name>Jim</name>,
correct += <age>25</age>;

TEST(4, correct, emps.employee[0].children());

END();