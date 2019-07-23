




































var gTestfile = 'regress-349602.js';

var BUGNUMBER = 349602;
var summary = 'decompilation of let with e4x literal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function () {\n    (let (a = 3) <x/>);\n}';
  try
  {
    var f = eval('(function () { let (a = 3) <x/>; })');
    actual = f + '';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
