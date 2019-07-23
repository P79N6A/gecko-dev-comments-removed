




































var gTestfile = 'regress-361558.js';

var BUGNUMBER = 361558;
var summary = 'Assertion: sprop->setter != js_watch_set';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = actual = 'No Crash';

({}.__proto__.watch('x', print)); ({}.watch('x', print));

reportCompare(expect, actual, summary);
