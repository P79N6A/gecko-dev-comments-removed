




































var gTestfile = 'regress-350666.js';

var BUGNUMBER = 350666;
var summary = 'decompilation for (;; delete expr)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { for(;; delete y.(x)) { } }
  actual = f + '';
  expect = 'function () {\n    for (;; y.(x), true) {\n    }\n}';
  compareSource(expect, actual, summary);

  try
  {
    eval('(' + expect + ')');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  expect = 'No Error';
  reportCompare(expect, actual, summary);

  f = function () { for(;; delete (y+x)) { } }
  actual = f + '';
  expect = 'function () {\n    for (;; y + x, true) {\n    }\n}';
  compareSource(expect, actual, summary);

  try
  {
    eval('(' + expect + ')');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  expect = 'No Error';
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
