




































var gTestfile = 'regress-459186.js';

var BUGNUMBER = 459186;
var summary = 'Do not crash in CheckDestructuring';
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
    for (var [,{y}] = 1 in []) {}
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
