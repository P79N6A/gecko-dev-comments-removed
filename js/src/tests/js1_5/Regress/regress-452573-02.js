




































var gTestfile = 'regress-452573-02.js';

var BUGNUMBER = 452573;
var summary = 'Do not assert with JIT: "(((rmask(rr) & FpRegs) != 0))"';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for(var j=0;j<5;++j) typeof void 1;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
