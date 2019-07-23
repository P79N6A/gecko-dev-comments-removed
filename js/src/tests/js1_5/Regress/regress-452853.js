




































var gTestfile = 'regress-452853.js';

var BUGNUMBER = 452853;
var summary = 'Do not crash in simple loop with array';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  
  jit(true);

  for (var j=0; j<4; ++j) { var a = ["", ""]; a[0] * a[1]; }
 
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
