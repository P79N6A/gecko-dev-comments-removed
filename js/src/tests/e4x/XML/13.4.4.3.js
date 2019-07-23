









































gTestfile = '13.4.4.3.js';

START("13.4.4.3 - XML appendChild()");

TEST(1, true, XML.prototype.hasOwnProperty("appendChild"));


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
    <employee id="2"><name>Sue</name><age>30</age></employee>
</employees>;

newEmp = <employee id="2"><name>Sue</name><age>30</age></employee>;

emps.appendChild(newEmp);
TEST(2, correct, emps);


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <employee id="0"><name>Jim</name><age>25</age><hobby>snorkeling</hobby></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

emps.employee.(name == "Jim").appendChild(<hobby>snorkeling</hobby>);
TEST(3, correct, emps);   

END();
