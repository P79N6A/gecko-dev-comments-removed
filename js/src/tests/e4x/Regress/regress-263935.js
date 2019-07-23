








































gTestfile = 'regress-263935.js';

START("Qualified names specifying all names in no namespace should only match names without namespaces");
printBugNumber(263935);

var ns1 = new Namespace("http://www.ns1.com");
var ns2 = new Namespace("http://www.ns2.com");
var none = new Namespace();

var x = <x/>
x.foo = "one";
x.ns1::foo = "two";
x.ns2::foo = "three";
x.bar = "four";

var actual = x.none::*;

var expected =
<>
  <foo>one</foo>
  <bar>four</bar>
</>

TEST(1, expected, actual);

END();