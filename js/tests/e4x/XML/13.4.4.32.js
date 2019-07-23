









































gTestfile = '13.4.4.32.js';

START("13.4.4.32 - XML replace()");

TEST(1, true, XML.prototype.hasOwnProperty("replace"));


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <requisition status="open" />
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

emps.replace(0, <requisition status="open" />);

TEST(2, correct, emps);



emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <requisition status="open" />
</employees>;

emps.replace("*", <requisition status="open" />);

TEST(3, correct, emps);



emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <requisition status="open" />
</employees>;

emps.replace("employee", <requisition status="open" />);

TEST(4, correct, emps);

END();
