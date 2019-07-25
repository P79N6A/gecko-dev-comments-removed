






var BUGNUMBER = 373082;
var summary = 'Simpler sharing of XML and XMLList functions';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var l;

expect = '<a/>';
l = <><a/></>;
actual = l.toXMLString();
TEST(1, expect, actual);

expect = '<b/>';
l.setName('b');
actual = l.toXMLString();
TEST(2, expect, actual);

expect = '<c/>';
XMLList.prototype.function::setName.call(l, 'c');
actual = l.toXMLString();
TEST(3, expect, actual);

expect = 't';
l = <><a>text</a></>;
actual = l.charAt(0);
TEST(4, expect, actual);

expect = "TypeError";

delete XML.prototype.function::toString;
var xml = <a>TEXT</a>;
var saveToString = Object.prototype.toString;
delete Object.prototype.toString;
try {
    actual = xml.toString();
} catch(ex) {
    actual = ex.name;
} finally {
    Object.prototype.toString = saveToString;
}
TEST(7, expect, actual);

expect = "TypeError";
try
{
    var x = <a><name/></a>;
    x.(name == "Foo");
    print(x.function::name());
}
catch(ex)
{
    actual = ex.name;
}
TEST(8, expect, actual);

try
{
    x = <a><name/></a>;
    x.(name == "Foo");
    print(x.name());
}
catch(ex)
{
    actual = ex.name;
}
TEST(9, expect, actual);

END();
