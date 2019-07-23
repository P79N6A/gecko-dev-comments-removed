




































var bug = 351120;
var summary = 'Incorrect error messages with yield expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'TypeError: [].z is not a function';
  actual = '';
  try
  {
    (function() { yield [].z({}); })().next();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
