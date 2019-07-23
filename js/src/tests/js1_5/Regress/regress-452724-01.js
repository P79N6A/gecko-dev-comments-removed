




































var gTestfile = 'regress-452724-01.js';

var BUGNUMBER = 452724;
var summary = 'Do not assert with JIT: (rmask(rr) & FpRegs) != 0';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function() { for (var j=0;j<5;++j) { (0/0) in this; } })()

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
