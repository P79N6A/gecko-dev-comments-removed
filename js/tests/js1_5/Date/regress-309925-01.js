




































var gTestfile = 'regress-309925-01.js';

var BUGNUMBER = 309925;
var summary = 'Correctly parse Date strings with HH:MM';
var actual = new Date('Sep 24, 11:58 105') + '';
var expect = new Date('Sep 24, 11:58:00 105') + '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
reportCompare(expect, actual, summary);
