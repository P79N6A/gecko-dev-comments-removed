





































gTestfile = 'regress-444608.js';

var summary = '13.3 QNames - call constructors directly';
var BUGNUMBER = 444608;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);


var x = <a><b/></a>;
QName = function() { return 10; };
x.replace("b", 10);

TEST(1, expect, actual);

END();
