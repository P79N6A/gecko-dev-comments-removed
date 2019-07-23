




































var gTestfile = 'regress-351597.js';

var BUGNUMBER = 351597;
var summary = 'decompilation of new expression with extra parens';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;
  f = function() { new ((x=a))(y) } 
  expect = 'function () {\n    new (x = a)(y);\n}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
