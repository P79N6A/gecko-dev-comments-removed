





































var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



printBugNumber(BUGNUMBER);
printStatus (summary);



jit(true);
__defineGetter__("x3", Function);
parseInt = x3;
parseInt.prototype = [];
for (var z = 0; z < 4; ++z) { new parseInt() }
jit(false);

reportCompare(expect, actual, summary);
