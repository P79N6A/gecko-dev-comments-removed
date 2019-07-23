




































var gTestfile = 'regress-350415.js';

var BUGNUMBER = 350415;
var summary = 'Do not assert with new Function("let /*")';
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
    new Function("let /*");
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
