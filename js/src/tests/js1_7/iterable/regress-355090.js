




































var gTestfile = 'regress-355090.js';

var BUGNUMBER = 355090;
var summary = 'Iterator(8) is a function';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'No Error';
  actual = 'No Error';
  try
  {
    Iterator(8);
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
