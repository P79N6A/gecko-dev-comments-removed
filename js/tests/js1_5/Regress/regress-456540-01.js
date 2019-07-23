




































var gTestfile = 'regress-456540-01.js';

var BUGNUMBER = 456540;
var summary = 'Do not assert with JIT: (m != JSVAL_INT) || isInt32(*vp)" with ((-1) % ""';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (var j = 0; j < 5; ++j) { var t = ((-1) % "" ); }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
