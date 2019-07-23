




































var gTestfile = 'regress-366288.js';

var BUGNUMBER = 366288;
var summary = 'Do not assert !SPROP_HAS_STUB_GETTER with __defineSetter__';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

this.__defineSetter__("x", function(){});
x = 3;

reportCompare(expect, actual, summary);
