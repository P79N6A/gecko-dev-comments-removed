




































var gTestfile = 'regress-385393-09.js';


var BUGNUMBER = 385393;
var summary = 'Regression test for bug 385393';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
eval("this.__defineSetter__('x', gc); this.watch('x', [].slice); x = 1;");

reportCompare(expect, actual, summary);
