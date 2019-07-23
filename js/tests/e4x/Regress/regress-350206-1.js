




































gTestfile = 'regress-350206-1.js';


var BUGNUMBER     = "350206";
var summary = "Order of destructuring, destructuring in the presence of " +
              "exceptions";
var actual, expect;

printBugNumber(BUGNUMBER);
START(summary);





var failed = "No crash";

try
{
  var t1 = <ns2:t1 xmlns:ns2="http://bar/ns2"/>;
  var n2 = new Namespace("http://ns2");
  t1.@n2::a1 = "a1 from ns2";

  t1.toXMLString();
}
catch (ex)
{
  failed = ex;
}

expect = "No crash";
actual = failed;

TEST(1, expect, actual);
