




































var gTestfile = 'regress-309925-02.js';

var BUGNUMBER = 309925;
var summary = 'Correctly parse Date strings with HH:MM(comment)';
var actual = new Date('Sep 24, 11:58(comment) 105') + '';
var expect = new Date('Sep 24, 11:58:00 (comment) 105') + '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
reportCompare(expect, actual, summary);
