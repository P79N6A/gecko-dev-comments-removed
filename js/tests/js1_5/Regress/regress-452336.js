




































var gTestfile = 'regress-452336.js';

var BUGNUMBER = 452336;
var summary = 'Do not assert with JIT: (slot) < (uint32)(obj)->dslots[-1]';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j = 0; j < 4; ++j) { [1].x++; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
