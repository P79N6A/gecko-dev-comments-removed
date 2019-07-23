




































var gTestfile = 'regress-285219.js';

var BUGNUMBER = 285219;
var summary = 'Do not crash on RangeError: reserved slot out of range';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var o = {hi: 'there'};
eval("var r = /re(1)(2)(3)/g", o);
 
reportCompare(expect, actual, summary);
