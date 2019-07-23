





































START("Null pointer deref crash deleting XML methods");

var bug = 331664;
var summary = 'Null pointer deref crash deleting XML methods';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

delete XML.prototype.attributes

TEST(1, expect, actual);

END();
