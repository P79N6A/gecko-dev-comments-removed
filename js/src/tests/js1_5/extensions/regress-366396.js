




































var gTestfile = 'regress-366396.js';

var BUGNUMBER = 366396;
var summary = 'Do not assert !SPROP_HAS_STUB_GETTER on Setter with %=';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
this.__defineSetter__("x", function() {}); x %= 5;

reportCompare(expect, actual, summary);
