




































var gTestfile = 'regress-351120.js';

var BUGNUMBER = 351120;
var summary = 'Incorrect error messages with yield expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /TypeError:.*(is not a function|Cannot find function).*/;
  actual = '';
  try
  {
    (function() { yield [].z({}); })().next();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
