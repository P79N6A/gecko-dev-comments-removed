









































START("13.4.4.33 - XML setChildren()");

TEST(1, true, XML.prototype.hasOwnProperty("setChildren"));

x =
<alpha>
    <bravo>one</bravo>
</alpha>;

correct = 
<alpha>
    <charlie>two</charlie>
</alpha>;

x.setChildren(<charlie>two</charlie>);

TEST(2, correct, x);


emps =
<employees>
    <employee id="0"><name>Jim</name><age>25</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

correct =
<employees>
    <employee id="0"><name>John</name><age>35</age></employee>
    <employee id="1"><name>Joe</name><age>20</age></employee>
</employees>;

emps.employee.(name == "Jim").setChildren(<name>John</name> + <age>35</age>);

TEST(3, correct, emps);

END();