




































var gTestfile = 'regress-452498-027.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '5';



  function f(x){function g(y)x+y;return g}
  g = f(2);

  actual = String(g(3));

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}



