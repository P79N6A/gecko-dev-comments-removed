





































var gTestfile = 'regress-140852.js';

var BUGNUMBER = 140852;
var summary = 'String(number) = xxxx:0000 for some numbers';
var actual = '';
var expect = '';


printBugNumber(BUGNUMBER);
printStatus (summary);

var value;
 
value = 99999999999;
expect = '99999999999';
actual = value.toString();
reportCompare(expect, actual, summary);

value = 100000000000;
expect = '100000000000';
actual = value.toString();
reportCompare(expect, actual, summary);

value = 426067200000;
expect = '426067200000';
actual = value.toString();
reportCompare(expect, actual, summary);

