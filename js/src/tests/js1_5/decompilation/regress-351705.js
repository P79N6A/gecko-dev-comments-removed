




































var gTestfile = 'regress-351705.js';

var BUGNUMBER = 351705;
var summary = 'decompilation of new unary expression';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function() { new (-y); x }
  actual = f + '';
  expect = 'function () {\n    new (- y);\n    x;\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
