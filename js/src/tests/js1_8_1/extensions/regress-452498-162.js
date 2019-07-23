




































var gTestfile = 'regress-452498-162.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



printBugNumber(BUGNUMBER);
printStatus (summary);



jit(true);
__defineGetter__("x3", Function);
undefined = x3;
undefined.prototype = [];
for (var z = 0; z < 4; ++z) { new undefined() }
jit(false);

reportCompare(expect, actual, summary);
