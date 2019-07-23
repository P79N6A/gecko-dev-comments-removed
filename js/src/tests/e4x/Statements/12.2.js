









































gTestfile = '12.2.js';

START("12.2 - For-in statement");


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

correct = new Array("Joe", "Sue", "Big Screen Television", "1299.99");

i = 0;
for (var n in e..name)
{
    TEST("1."+i, String(i), n);
    i++;
}
TEST("1.count", 2, i);



order =
<order>
    <customer>
        <name>John Smith</name>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
</order>;

i = 0;
for (var child in order.item)
{
    TEST("2."+i, String(i), child);
    i++
}
TEST("2.count", 2, i);

i = 0;
for (var child in order.item[0].*)
{
    TEST("3."+i, String(i), child);
    i++
}

TEST("3.count", 2, i);

END();
