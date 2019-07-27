





var BUGNUMBER = 328556;
var summary = 'Do not Assert: growth == (size_t)-1 || (nchars + 1) * sizeof(char16_t) == growth, in jsarray.c';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var D = [];
D.foo = D;
uneval(D);
 
reportCompare(expect, actual, summary);
