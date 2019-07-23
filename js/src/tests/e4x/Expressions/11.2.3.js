









































gTestfile = '11.2.3.js';

START("11.2.3 - XML Descendant Accessor");

e =
<employees>
    <employee id="1"><name>Joe</name><age>20</age></employee>
    <employee id="2"><name>Sue</name><age>30</age></employee>
</employees>   

names = e..name;

correct =
<name>Joe</name> +
<name>Sue</name>;

TEST(1, correct, names);

END();