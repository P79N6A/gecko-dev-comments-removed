




































var gTestfile = 'regress-350242.js';

var BUGNUMBER = 350242;
var summary = 'decompilation of delete 0x11.x';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { delete 0x11.x }
  expect = 'function () {\n    delete (17).x;\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function () { delete (17).x }
  expect = 'function () {\n    delete (17).x;\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
