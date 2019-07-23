









































gTestfile = '10.3.1.js';

START("10.3.1 - toXML applied to String type");

john = "<employee><name>John</name><age>25</age></employee>";
sue = "<employee><name>Sue</name><age>32</age></employee>";
tagName = "employees";
employees = new XML("<" + tagName + ">" + john + sue + "</" + tagName + ">");

correct =
<employees>
    <employee><name>John</name><age>25</age></employee>
    <employee><name>Sue</name><age>32</age></employee>
</employees>;

TEST(1, correct, employees);

END();