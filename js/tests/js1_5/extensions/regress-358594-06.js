




































var gTestfile = 'regress-358594-06.js';


var BUGNUMBER = 358594;
var summary = 'Do not crash on uneval(this).';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);


f = function () { };
f.hhhhhhhhh = this; 
this.m setter = f; 
uneval(this);
reportCompare(expect, actual, summary);
