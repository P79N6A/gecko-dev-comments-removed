





































var BUGNUMBER = 455464;
var summary = 'Do not assert with JIT: !TRACE_RECORDER(cx) ^ (jumpTable == recordingJumpTable)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

a=b=c=d=0; this.__defineGetter__('g', gc); for each (y in this);

jit(false);

reportCompare(expect, actual, summary);
