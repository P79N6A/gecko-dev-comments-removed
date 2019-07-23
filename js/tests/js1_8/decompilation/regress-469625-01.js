




































var gTestfile = 'regress-469625-01.js';

var BUGNUMBER = 469625;
var summary = 'decompile [a, b, [c0, c1]] = [x, x, x];';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(x) {
    var [a, b, [c0, c1]] = [x, x, x];
  }

  expect = 'function f(x) { var [a, b, [c0, c1]] = [x, x, x]; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
