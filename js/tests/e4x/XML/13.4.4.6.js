









































gTestfile = '13.4.4.6.js';

START("13.4.4.6 - XML child()");

TEST(1, true, XML.prototype.hasOwnProperty("child"));

emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employee id="0"><name>Jim</name><age>25</age></employee> +
<employee id="1"><name>Joe</name><age>20</age></employee>;


TEST(2, correct, emps.child("employee"));

TEST(3, <name>Joe</name>, emps.employee[1].child("name"));

correct = <employee id="1"><name>Joe</name><age>20</age></employee>;

TEST(4, correct, emps.child(1));

END();
