




































var gTestfile = 'regress-452495.js';

var BUGNUMBER = 452495;
var summary = 'Do not crash with JIT: @ TraceRecorder::getThis';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

for (var j = 0; j < 4; ++j) { try { new 1(this); } catch(e) { } }

jit(false);

reportCompare(expect, actual, summary);
