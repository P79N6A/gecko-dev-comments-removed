




































var bug = 355052;
var summary = 'Do not crash with valueOf:gc and __iterator__';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'TypeError: NaN is not a function';
  actual = 'No Error';
  try
  {
    ( {valueOf: gc} - [function(){}].__iterator__ )();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
