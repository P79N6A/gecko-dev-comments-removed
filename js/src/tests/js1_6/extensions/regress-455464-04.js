




































var gTestfile = 'regress-455464-04.js';

var BUGNUMBER = 455464;
var summary = 'Do not assert with JIT, gczeal 2: !TRACE_RECORDER(cx) ^ (jumpTable == recordingJumpTable)';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof gczeal == 'undefined')
{
  expect = actual = 'Test requires gczeal, skipped.';
}
else
{
  jit(true);
  gczeal(2);

  a=b=c=d=0; this.__defineGetter__('g', gc); for each (y in this);

  gczeal(0);
  jit(false);
}

reportCompare(expect, actual, summary);
