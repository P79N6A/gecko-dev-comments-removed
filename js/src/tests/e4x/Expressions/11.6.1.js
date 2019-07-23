









































gTestfile = '11.6.1.js';

START("11.6.1 - XML Assignment");


order =
<order>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

correct =
<order>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="1.23">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

order.item[1].@id = 1.23;
TEST(1, correct, order);


order =
<order>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

correct =
<order>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2" newattr="new value">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

order.item[1].@newattr = "new value";
TEST(2, correct, order);


order =
<order>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

order.@allids = order.item.@id;   
TEST_XML(3, "1 2 3 4", order.@allids);


order =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;


order.*[0] =
<customer>
    <name>Fred</name>
    <address>123 Foobar Ave.</address>
    <city>Bellevue</city>
    <state>WA</state>
</customer>;

correct =
<order>
    <customer>
       <name>Fred</name>
       <address>123 Foobar Ave.</address>
       <city>Bellevue</city>
       <state>WA</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

TEST(4, correct, order);



order =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

correct =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item>item one</item>
    <item>item two</item>
    <item>item three</item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

order.item[0] = <item>item one</item> +
           <item>item two</item> +
           <item>item three</item>;

TEST(5, correct, order);


order =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

correct =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">A Text Node</item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

order.item[1] = "A Text Node";

TEST(6, correct, order);



order =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
</order>;

correct =
<order>
    <customer>
        <name>John</name>
        <address>948 Ranier Ave.</address>
        <city>Portland</city>
        <state>OR</state>
    </customer>
    <item id="1">
        <description>Big Screen Television</description>
        <price>1299.99</price>
    </item>
    <item id="2">
        <description>DVD Player</description>
        <price>399.99</price>
    </item>
    <item id="3">
        <description>CD Player</description>
        <price>199.99</price>
    </item>
    <item id="4">
        <description>8-Track Player</description>
        <price>69.99</price>
    </item>
    <item>new item</item>
</order>;

order.*[order.*.length()] = <item>new item</item>;

TEST(7, correct, order);


item =
<item>
    <description>Big Screen Television</description>
    <price>1299.99</price>
</item>

correct =
<item>
    <description>Big Screen Television</description>
    <price>99.95</price>
</item>

item.price = 99.95;

TEST(8, item, correct);


item =
<item>
    <description>Big Screen Television</description>
    <price>1299.99</price>
</item>

correct =
<item>
    <description>Mobile Phone</description>
    <price>1299.99</price>
</item>

item.description = "Mobile Phone";

TEST(9, item, correct);

END();
