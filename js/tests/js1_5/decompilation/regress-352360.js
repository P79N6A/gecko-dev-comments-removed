




































var gTestfile = 'regress-352360.js';

var BUGNUMBER = 352360;
var summary = 'Decompilation of negative 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { return -0 };
  expect = 'function() { return -0; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = -Infinity;
  actual = 8 / f();
  reportCompare(expect, actual, summary + ': 8 / f()');

  expect = -Infinity;
  actual = 8 / eval('(' + f + ')')();
  reportCompare(expect, actual, summary + ': 8 / eval("" + f)()');

  exitFunc ('test');
}
