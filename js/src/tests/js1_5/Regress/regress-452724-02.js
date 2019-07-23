




































var gTestfile = 'regress-452724-02.js';

var BUGNUMBER = 452724;
var summary = 'Do not crash with JIT: @TraceRecorder::getThis';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j=0;j<5;++j) { (0/0) in this; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
