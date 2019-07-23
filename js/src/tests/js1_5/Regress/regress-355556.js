




































var gTestfile = 'regress-355556.js';

var BUGNUMBER = 355556;
var summary = 'Do not crash with eval(..., arguments)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: "foo".b is not a function';
  try
  {
    (function () { eval("'foo'.b()", arguments) })();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
