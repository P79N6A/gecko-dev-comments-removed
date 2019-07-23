




































var gTestfile = 'regress-420612.js';

var BUGNUMBER = 420612;
var summary = 'Do not assert: obj == pobj';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
this.__proto__ = []; 
this.unwatch("x");
reportCompare(expect, actual, summary);
