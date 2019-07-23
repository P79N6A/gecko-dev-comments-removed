




































var gTestfile = 'regress-303277.js';

var BUGNUMBER = 303277;
var summary = 'Do not crash with crash with a watchpoint for __proto__ property ';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var o = {};
o.watch("__proto__", function(){return null;});
o.__proto__ = null;
 
reportCompare(expect, actual, summary);
