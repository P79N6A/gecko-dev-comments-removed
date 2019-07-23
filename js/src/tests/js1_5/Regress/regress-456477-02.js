




































var gTestfile = 'regress-456477-02.js';

var BUGNUMBER = 456477;
var summary = 'Do not assert with JIT: (m != JSVAL_INT) || isInt32(*vp)" with (0/0)%(-1)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  (function() { for (var j = 0; j < 5; ++j) { (0 / 0) % (-1); } })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
