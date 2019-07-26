





var BUGNUMBER = 311071;
var summary = 'treat &lt;! as the start of a comment to end of line';
var actual = '';
var expect = '';


printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'foo';
actual = 'foo'; <!-- comment hack -->; actual = 'bar'; 
reportCompare(expect, actual, summary);
