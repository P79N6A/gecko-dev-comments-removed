




































var gTestfile = 'regress-481989.js';

var BUGNUMBER = 481989;
var summary = 'TM: Do not assert: SPROP_HAS_STUB_SETTER(sprop)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

y = this.watch("x", function(){}); for each (let y in ['', '']) x = y;

jit(true);

reportCompare(expect, actual, summary);
