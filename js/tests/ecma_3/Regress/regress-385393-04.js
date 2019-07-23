




































var gTestfile = 'regress-385393-04.js';

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
    'a'.replace(/a/g, eval);
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
