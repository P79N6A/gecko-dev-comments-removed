




































var gTestfile = 'regress-308806-01.js';

var BUGNUMBER = 308806;
var summary = 'Object.prototype.toLocaleString() should track Object.prototype.toString() ';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var o = {toString: function() { return 'foo'; }};

expect = o.toString();
actual = o.toLocaleString();
 
reportCompare(expect, actual, summary);
