





































var bug = 374112;
var summary = 'E4X Do not assert with xml.setName(...)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

<a/>.setName(<b/>.name());

TEST(1, expect, actual);
END();
