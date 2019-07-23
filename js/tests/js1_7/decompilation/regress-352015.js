




































var gTestfile = 'regress-352015.js';

var BUGNUMBER = 352015;
var summary = 'decompilation of yield expressions with parens';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { (yield).a }
  actual = f + '';
  expect = 'function () {\n    (yield).a;\n}';
  compareSource(expect, actual, summary);

  f = function() { 3 + (yield 4) }
  actual = f + '';
  expect = 'function () {\n    3 + (yield 4);\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
