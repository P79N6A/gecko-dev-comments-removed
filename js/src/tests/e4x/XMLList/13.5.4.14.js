









































gTestfile = '13.5.4.14.js';

START("13.5.4.14 - XMLList length()");

TEST(1, true, XMLList.prototype.hasOwnProperty("length"));

x = <><alpha>one</alpha></>;

TEST(2, 1, x.length());

x = <><alpha>one</alpha><bravo>two</bravo></>;

TEST(2, 2, x.length());

emps =
<employees>
    <employee>
        <name>John</name>
    </employee>
    <employee>
        <name>Sue</name>
    </employee>
</employees>   

correct =
<employees>
    <employee>
        <prefix>Mr.</prefix>
        <name>John</name>
    </employee>
    <employee>
        <name>Sue</name>
    </employee>
</employees>   

TEST(3,2,emps..name.length());

END();
