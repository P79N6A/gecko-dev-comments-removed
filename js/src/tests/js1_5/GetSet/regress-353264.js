




































var gTestfile = 'regress-353264.js';

var BUGNUMBER = 353264;
var summary = 'Do not crash defining getter';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

this.x getter= function () { }; export x; x;
 
reportCompare(expect, actual, summary);
