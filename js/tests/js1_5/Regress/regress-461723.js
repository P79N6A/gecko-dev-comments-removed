




































var gTestfile = 'regress-461723.js';

var BUGNUMBER = 461723;
var summary = 'Do not assert: (m != JSVAL_INT) || isInt32(*vp)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (var j = 0; j < 30; ++j) { (0 + void 0) && 0; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
