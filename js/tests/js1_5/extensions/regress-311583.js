




































var gTestfile = 'regress-311583.js';

var BUGNUMBER = 311583;
var summary = 'uneval(array) should use elision for holes';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var a = new Array(3);
a[0] = a[2] = 0;

actual = uneval(a);
expect = '[0, , 0]'; 

reportCompare(expect, actual, summary);
