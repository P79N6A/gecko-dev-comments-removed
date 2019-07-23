




































var gTestfile = 'regress-457456.js';

var BUGNUMBER = 457456;
var summary = 'Do not assert with JIT: cond->isCond()';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (var j = 0; j < 4; ++j) { if (undefined < false) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
