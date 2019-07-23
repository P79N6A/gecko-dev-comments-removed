




































var bug = 354945;
var summary = 'Iterator(8) is a function';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
