





































var bug = 374116;
var summary = 'Crash with <a/>.@b[1] = 2;';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

<a/>.@b[1] = 2;

TEST(1, expect, actual);
END();
