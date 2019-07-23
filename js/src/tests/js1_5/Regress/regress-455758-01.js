




































var gTestfile = 'regress-455758-01.js';

var BUGNUMBER = 455758;
var summary = 'Do not assert: (m != JSVAL_INT) || isInt32(*vp)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function() { for (var j = 0; j < 5; ++j) { var t = 3 % (-0); } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
