




































gTestfile = 'regress-313799.js';

var summary = 'Do not crash on XMLListInitializer.child(0)';
var BUGNUMBER = 313799;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var val = <><t/></>.child(0);

TEST(1, expect, actual);
TEST(2, 'xml',  typeof val);

END();
