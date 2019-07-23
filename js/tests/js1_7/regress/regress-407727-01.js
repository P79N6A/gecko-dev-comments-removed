




































var gTestfile = 'regress-407727-01.js';

var BUGNUMBER = 407727;
var summary = 'let Object global redeclaration';
var actual = '';
var expect = 1;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
let Object = 1;
actual = Object;
reportCompare(expect, actual, summary);
