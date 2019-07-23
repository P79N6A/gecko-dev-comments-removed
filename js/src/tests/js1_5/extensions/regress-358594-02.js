




































var gTestfile = 'regress-358594-02.js';


var BUGNUMBER = 358594;
var summary = 'Do not crash on uneval(this).';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);


function f() { } 
f.__proto__ = this; 
this.m setter = f; 
uneval(this);
reportCompare(expect, actual, summary);
