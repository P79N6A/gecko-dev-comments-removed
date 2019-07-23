




































var gTestfile = 'regress-311071.js';

var BUGNUMBER = 311071;
var summary = 'treat &lt;! as the start of a comment to end of line if not e4x';
var actual = '';
var expect = '';


printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'foo';
actual = 'foo'; <!-- comment hack -->; actual = 'bar'; 
reportCompare(expect, actual, summary);
