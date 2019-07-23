




































var gTestfile = 'regress-452713.js';

var BUGNUMBER = 452713;
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

  for (var j = 0; j < 5; ++j) { if (''[-1]) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
