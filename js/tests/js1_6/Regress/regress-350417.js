




































var gTestfile = 'regress-350417.js';

var BUGNUMBER = 350417;
var summary = 'Do not crash decompiling "is not function" msg';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: y.a = [2 for each (p in [])] is not a function';
  try
  {
    eval('y = {}; (y.a = [2 for each (p in [])])();');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
