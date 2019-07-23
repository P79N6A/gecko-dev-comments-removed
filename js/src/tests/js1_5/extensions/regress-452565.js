




































var gTestfile = 'regress-452565.js';

var BUGNUMBER = 452565;
var summary = 'Do not assert with JIT: !(sprop->attrs & JSPROP_READONLY)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

const c; (function() { for (var j=0;j<5;++j) { c = 1; } })();

jit(false);

reportCompare(expect, actual, summary);
