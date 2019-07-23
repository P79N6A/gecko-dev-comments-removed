




































var gTestfile = 'regress-452338.js';

var BUGNUMBER = 452338;
var summary = 'Do not assert with JIT: obj2 == obj';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j = 0; j < 4; ++j) __count__ = 3;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
