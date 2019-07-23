




































START("13.5.4.4 - XMLList.prototype.child");

var bug = 313799;
var summary = 'Do not crash on XMLListInitializer.child(0)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var val = <><t/></>.child(0);

TEST(1, expect, actual);
TEST(2, 'xml',  typeof val);

END();
