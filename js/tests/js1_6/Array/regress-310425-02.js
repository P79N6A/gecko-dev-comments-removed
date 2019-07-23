




































var gTestfile = 'regress-310425-02.js';

var BUGNUMBER = 310425;
var summary = 'Array.indexOf/lastIndexOf edge cases';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = -1;
actual = Array(1).indexOf(1);
reportCompare(expect, actual, summary);
