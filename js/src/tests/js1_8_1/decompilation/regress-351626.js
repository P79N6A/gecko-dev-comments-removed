




































var gTestfile = 'regress-351626.js';

var BUGNUMBER = 351626;
var summary = 'decompilation of if(lamda)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { if (function () {}) { g(); } }
  actual = f + '';
  expect = 'function () {\n  g();\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
