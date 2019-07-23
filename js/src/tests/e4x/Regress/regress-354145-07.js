





































gTestfile = 'regress-354145-07.js';

var BUGNUMBER = 354145;
var summary = 'Immutable XML';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);


var a = <a><b/></a>
try {
  a.insertChildAfter(a.b[0], {toString: function() { throw 1; }});
} catch (e) { }

for each (var i in a) { }

TEST(1, expect, actual);

END();
