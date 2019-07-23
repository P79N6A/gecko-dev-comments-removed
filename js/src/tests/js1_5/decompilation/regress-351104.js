




































var gTestfile = 'regress-351104.js';

var BUGNUMBER = 351104;
var summary = 'decompilation of for with ternary as initializer';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function () { for((0 ? 2 : ({})); ; ) { } }
  expect = 'function () {\n    for ({};;) {\n    }\n}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
