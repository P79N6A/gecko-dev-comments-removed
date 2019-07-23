




































var gTestfile = 'regress-452170.js';

var BUGNUMBER = 452170;
var summary = 'Do not assert with JIT: (*m != JSVAL_INT) || isInt32(*vp)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for (var j = 0; j < 4; ++j) { (-0).toString(); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
