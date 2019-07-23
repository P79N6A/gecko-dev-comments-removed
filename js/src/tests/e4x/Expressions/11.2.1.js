









































gTestfile = '11.2.1.js';

START("11.2.1 - Property Accessors");

order =
<order id="123456" timestamp="Mon Mar 10 2003 16:03:25 GMT-0800 (PST)">
    <customer>
        <firstname>John</firstname>
        <lastname>Doe</lastname>
    </customer>
    <item>
        <description>Big Screen Television</description>
        <price>1299.99</price>
        <quantity>1</quantity>
    </item>
</order>;   

correct =
<customer>
    <firstname>John</firstname>
    <lastname>Doe</lastname>
</customer>;

TEST(1, correct, order.customer);
TEST_XML(2, 123456, order.@id);

correct =
<item>
    <description>Big Screen Television</description>
    <price>1299.99</price>
    <quantity>1</quantity>
</item>

TEST(3, correct, order.children()[1]);

correct =
<customer>
    <firstname>John</firstname>
    <lastname>Doe</lastname>
</customer> +
<item>
    <description>Big Screen Television</description>
    <price>1299.99</price>
    <quantity>1</quantity>
</item>;


TEST(4, correct, order.*);

correct = new XMLList();
correct += new XML("123456");
correct += new XML("Mon Mar 10 2003 16:03:25 GMT-0800 (PST)");
TEST(5, correct, order.@*);

order = <order>
        <customer>
            <firstname>John</firstname>
            <lastname>Doe</lastname>
        </customer>
        <item id="3456">
            <description>Big Screen Television</description>
            <price>1299.99</price>
            <quantity>1</quantity>
        </item>
        <item id="56789">
            <description>DVD Player</description>
            <price>399.99</price>
            <quantity>1</quantity>
        </item>
        </order>;

correct =
<description>Big Screen Television</description> +
<description>DVD Player</description>;

TEST(6, correct, order.item.description);

correct = new XMLList();
correct += new XML("3456");
correct += new XML("56789");
TEST(7, correct, order.item.@id);

correct =
<item id="56789">
    <description>DVD Player</description>
    <price>399.99</price>
    <quantity>1</quantity>
</item>

TEST(8, correct, order.item[1]);

correct =
<description>Big Screen Television</description> +
<price>1299.99</price> +
<quantity>1</quantity> +
<description>DVD Player</description> +
<price>399.99</price> +
<quantity>1</quantity>;

TEST(9, correct, order.item.*);

correct=
<price>1299.99</price>;

TEST(10, correct, order.item.*[1]);


order = <order>
        <customer>
            <firstname>John</firstname>
            <lastname>Doe</lastname>
        </customer>
        <item id="3456">
            <description>Big Screen Television</description>
            <price>1299.99</price>
            <quantity>1</quantity>
        </item>
        <item id="56789">
            <description>DVD Player</description>
            <price>399.99</price>
            <quantity>1</quantity>
        </item>
        </order>;


TEST(11, order, order[0]);


TEST(12, undefined, order[1]);

END();
