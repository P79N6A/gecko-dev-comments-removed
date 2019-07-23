




































var gTestfile = 'regress-452491.js';

var BUGNUMBER = 452491;
var summary = 'Do not crash with JIT: with new';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j=0;j<5;++j) (new (function(q) q)).a;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
