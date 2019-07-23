




































var gTestfile = 'regress-385393-01.js';


var BUGNUMBER = 385393;
var summary = 'Regression test for bug 385393';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    [].map(1 for (x in []));
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
