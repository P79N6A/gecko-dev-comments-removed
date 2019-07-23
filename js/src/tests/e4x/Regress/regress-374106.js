





































gTestfile = 'regress-374106.js';

var BUGNUMBER = 374106;
var summary = 'e4x XMLList.contains execution halts with complex match';
var actual = 'No Error';
var expect = 'No Error';

printBugNumber(BUGNUMBER);
START(summary);

<><x><y/></x></>.contains(<x><y/></x>); 3;

TEST(1, expect, actual);
END();
