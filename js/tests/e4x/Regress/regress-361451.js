





































var bug = 361451;
var summary = 'Do not crash with E4X, watch, import';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var obj = <z><yyy/></z>;
obj.watch('x', print);
try { import obj.yyy; } catch(e) { }
obj = undefined;
gc();

TEST(1, expect, actual);

END();
