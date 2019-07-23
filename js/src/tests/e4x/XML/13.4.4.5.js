









































gTestfile = '13.4.4.5.js';

START("13.4.4.5 - XML attributes()");

TEST(1, true, XML.prototype.hasOwnProperty("attributes"));

x =
<alpha attr1="value1" attr2="value2" attr3="value3">
    <bravo>one</bravo>
</alpha>;

TEST(2, "xml", typeof(x.attributes()));


x =
<alpha attr1="value1" attr2="value2" attr3="value3">
    <bravo>one</bravo>
</alpha>;

correct = new Array("value1", "value2", "value3");
i = 0;

for each (var a in x.attributes())
{
    TEST_XML(i + 3, correct[i], a);
    i++;
}

END();