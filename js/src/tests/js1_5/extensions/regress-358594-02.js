






































var BUGNUMBER = 358594;
var summary = 'Do not crash on uneval(this).';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);


function f() { } 
f.__proto__ = this; 
Object.defineProperty(this, "m", { set: f, enumerable: true, configurable: true });
uneval(this);
reportCompare(expect, actual, summary);
