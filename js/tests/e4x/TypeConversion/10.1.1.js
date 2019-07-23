









































gTestfile = '10.1.1.js';

START("10.1.1 - XML.toString");

var n = 0;
var expect;
var actual;
var xml;



var order =
<order>
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

var name = order.customer.firstname + " " + order.customer.lastname;

TEST(++n, "John Doe", name);

var total = order.item.price * order.item.quantity;

TEST(++n, 1299.99, total);



printStatus("test empty.toString()");

xml = new XML();
expect = '';
actual = xml.toString();
TEST(++n, expect, actual);

printStatus("test attribute.toString()");

xml = <foo bar="baz"/>;
var attr = xml.@bar;
expect = "baz";
actual = attr.toString();
TEST(++n, expect, actual);

printStatus("test text.toString()");

xml = new XML("this is text");
expect = "this is text";
actual = xml.toString();
TEST(++n, expect, actual);

printStatus("test simpleContent.toString()");

xml = <foo>bar</foo>;
expect = "bar";
actual = xml.toString();
TEST(++n, expect, actual);

var truefalse = [true, false];

for (var ic = 0; ic < truefalse.length; ic++)
{
    for (var ip = 0; ip < truefalse.length; ip++)
    {
        XML.ignoreComments = truefalse[ic];
        XML.ignoreProcessingInstructions = truefalse[ip];

        xml = <foo><!-- comment1 --><?pi1 ?>ba<!-- comment2 -->r<?pi2 ?></foo>;
        expect = "bar";
        actual = xml.toString();
        TEST(++n, expect, actual);
    }
}

printStatus("test nonSimpleContent.toString() == " +
            "nonSimpleContent.toXMLString()");

var indents = [0, 4];

for (var pp = 0; pp < truefalse.length; pp++)
{
    XML.prettyPrinting = truefalse[pp];
    for (var pi = 0; pi < indents.length; pi++)
    {
        XML.prettyIndent = indents[pi];
  
        expect = order.toXMLString();
        actual = order.toString();
        TEST(++n, expect, actual);
    }
}

END();

