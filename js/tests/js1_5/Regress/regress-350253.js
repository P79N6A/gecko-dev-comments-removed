




































var gTestfile = 'regress-350253.js';

var BUGNUMBER = 350253;
var summary = 'Do not assert on (g()) = 3';
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
    (g()) = 3;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
