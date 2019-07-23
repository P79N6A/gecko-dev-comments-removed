




































var gTestfile = 'regress-478314.js';

var BUGNUMBER = 478314;
var summary = 'Do not assert: Assertion failed: "need a way to EOT now, since this is trace end": 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var z = 0; z < 2; ++z) { switch(false & 1e-81) {} }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
