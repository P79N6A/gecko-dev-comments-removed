




































var gTestfile = 'regress-455408.js';

var BUGNUMBER = 455408;
var summary = 'Do not assert with JIT: "Should not move data from GPR to XMM": false';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j = 0; j < 5; ++j) { if (({}).__proto__ = 1) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
