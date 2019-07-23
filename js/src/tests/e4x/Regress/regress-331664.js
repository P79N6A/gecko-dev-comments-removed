





































gTestfile = 'regress-331664.js';

var summary = "Null pointer deref crash deleting XML methods";
var BUGNUMBER = 331664;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

delete XML.prototype.attributes

TEST(1, expect, actual);

END();
