





var BUGNUMBER = 355052;
var summary = 'Do not crash with valueOf:gc and __iterator__';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /TypeError: .+ is not a function/;
  actual = 'No Error';
  try
  {
    ( {valueOf: gc} - [function(){}].__iterator__ )();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
