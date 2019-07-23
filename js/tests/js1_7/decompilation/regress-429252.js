




































var gTestfile = 'regress-429252.js';

var BUGNUMBER = 429252;
var summary = 'trap should not change decompilation of { let x }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() { { let x } }

  expect = 'function f() { { let x; } }';
  actual = f + '';
  compareSource(expect, actual, summary + ': before trap');

  if (typeof trap == 'function')
  {
    trap(f, 0, "");

    actual = f + '';
    compareSource(expect, actual, summary + ': after trap');
  }
  exitFunc ('test');
}
