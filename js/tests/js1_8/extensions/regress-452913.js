




































var gTestfile = 'regress-452913.js';

var BUGNUMBER = 452913;
var summary = 'Do not crash with defined getter and for (let)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
(this.__defineGetter__("x", function (x)'foo'.replace(/o/g, [1].push)));
for(let y in [,,,]) for(let y in [,,,]) x = x;

reportCompare(expect, actual, summary);
