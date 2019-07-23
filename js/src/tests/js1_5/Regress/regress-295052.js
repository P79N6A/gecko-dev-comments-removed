




































var gTestfile = 'regress-295052.js';

var BUGNUMBER = 295052;
var summary = 'Do not crash when apply method is called on String.prototype.match';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

"".match.apply();
 
reportCompare(expect, actual, summary);
