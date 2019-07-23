




































var gTestfile = 'regress-351219.js';

var BUGNUMBER = 351219;
var summary = 'Decompilation of immutable infinity, NaN';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { return 2e308 };
  expect = 'function () { return 1 / 0; }';
  actual = f + '';
  compareSource(expect, actual, summary + ' decompile Infinity as 1/0');

  f = function () { var NaN = 1 % 0; return NaN };
  expect = 'function () { var NaN = 0 / 0; return NaN; }';
  actual = f + '';
  compareSource(expect, actual, summary + ': decompile NaN as 0/0');

  exitFunc ('test');
}
