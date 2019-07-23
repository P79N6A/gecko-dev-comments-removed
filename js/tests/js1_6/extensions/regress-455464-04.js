




































var gTestfile = 'regress-455464-04.js';

var BUGNUMBER = 455464;
var summary = 'Do not assert with JIT, gczeal 2: !TRACE_RECORDER(cx) ^ (jumpTable == recordingJumpTable)';
var actual = 'No Crash';
var expect = 'No Crash';

a=b=c=d=0; this.__defineGetter__('g', gc); for each (y in this);


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);
  gczeal(2);

  a=b=c=d=0; this.__defineGetter__('g', gc); for each (y in this);

  gczeal(0);
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
