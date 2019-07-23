




































var bug = 366292;
var summary = '__defineSetter__ and JSPROP_SHARED regression';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = 'undefined';
this.__defineSetter__("x", function(){}); 
actual = String(x);

reportCompare(expect, actual, summary);
