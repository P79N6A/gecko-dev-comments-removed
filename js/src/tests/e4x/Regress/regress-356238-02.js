





































gTestfile = 'regress-356238-02.js';

var BUGNUMBER = 356238;
var summary = 'bug 356238';
var actual = 'No Duplicate';
var expect = 'No Duplicate';

printBugNumber(BUGNUMBER);
START(summary);

var xml = <xml><child><a/><b/></child></xml>;
var child = xml.child[0];

try {
    child.insertChildAfter(null, xml.child);
    actual = "insertChildAfter succeeded when it should throw an exception";
}
catch (e) {
}

if (child.a[0] === child.a[1])
    actual = 'Duplicate detected';

TEST(1, expect, actual);
END();
