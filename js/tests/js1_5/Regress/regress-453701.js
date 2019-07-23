




































var gTestfile = 'regress-453701.js';

var BUGNUMBER = 453701;
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

  (function() { for (var j = 0; j < 5; ++j) { (1).hasOwnProperty(""); } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
