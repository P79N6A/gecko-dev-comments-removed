









































gTestfile = '13.2.5.js';

START("13.2.5 - Properties of Namespace Instances");

n = new Namespace("ns", "http://someuri");
TEST(1, true, n.hasOwnProperty("prefix"));
TEST(2, true, n.hasOwnProperty("uri"));
TEST(3, true, n.propertyIsEnumerable("prefix"));
TEST(4, true, n.propertyIsEnumerable("uri"));

var prefixCount = 0;
var uriCount = 0;
var p;
for(p in n)
{
    if(p == "prefix") prefixCount++;
    if(p == "uri") uriCount++;
}

TEST(5, 1, prefixCount);
TEST(6, 1, uriCount);
TEST(7, "ns", n.prefix);
TEST(8, "http://someuri", n.uri);

END();