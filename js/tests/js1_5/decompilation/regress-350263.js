




































var gTestfile = 'regress-350263.js';

var BUGNUMBER = 350263;
var summary = 'decompilation of labeled if(1);';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { L: if(1); }
  expect = 'function () {\n    L: {\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
