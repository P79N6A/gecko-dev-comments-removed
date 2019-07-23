









































gTestfile = '11.6.3.js';

START("11.6.3 - Compound Assignment");


e =
<employees>
    <employee id="1">
        <name>Joe</name>
        <age>20</age>
    </employee>
    <employee id="2">
        <name>Sue</name>
        <age>30</age>
    </employee>
</employees>;

correct =
<employees>
    <employee id="1">
        <name>Joe</name>
        <age>20</age>
    </employee>
    <employee id="3">
        <name>Fred</name>
    </employee>
    <employee id="4">
        <name>Carol</name>
    </employee>
    <employee id="2">
        <name>Sue</name>
        <age>30</age>
    </employee>
</employees>;
   
e.employee[0] += <employee id="3"><name>Fred</name></employee> +
    <employee id="4"><name>Carol</name></employee>;
   
TEST(1, correct, e);


e =
<employees>
    <employee id="1">
        <name>Joe</name>
        <age>20</age>
    </employee>
    <employee id="2">
        <name>Sue</name>
        <age>30</age>
    </employee>
</employees>;

correct =
<employees>
    <employee id="1">
        <name>Joe</name>
        <age>20</age>
    </employee>
    <employee id="2">
        <name>Sue</name>
        <age>30</age>
    </employee>
    <employee id="3">
        <name>Fred</name>
    </employee>
    <employee id="4">
        <name>Carol</name>
    </employee>
</employees>;

e.employee[1] += <employee id="3"><name>Fred</name></employee> +
    <employee id="4"><name>Carol</name></employee>;
TEST(2, correct, e);
       

END();