





































gTestfile = 'regress-374025.js';

var summary = 'Do not crash with XML.prettyIndent = 2147483648';
var BUGNUMBER = 374025;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

XML.prettyIndent = 2147483648; 
uneval(<x><y/></x>);

TEST(1, expect, actual);

END();
