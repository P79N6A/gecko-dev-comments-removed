





































gTestfile = 'regress-356238-03.js';

var BUGNUMBER = 356238;
var summary = 'bug 356238';
var actual = 'No Error';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
START(summary);

var xml = <xml><child><a/></child></xml>;
var child = xml.child[0];

try {
  child.insertChildBefore(null, xml.child);
  actual = "insertChildBefore succeded when it should throw an exception";
} catch (e) {
}

var list = child.*;
var text = uneval(list[1]);
if (!/(undefined|\(void 0\))/.test(text))
  throw "child got unexpected second element: "+text;

TEST(1, expect, actual);
END();
