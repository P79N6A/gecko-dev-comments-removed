




































var gTestfile = 'regress-341496.js';

var BUGNUMBER = 341496;
var summary = 'Iterators: check that adding properties does not crash';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var iter = Iterator({});
for (var i = 0; i != 10*1000; ++i)
  iter[i] = i;
 
reportCompare(expect, actual, summary);
