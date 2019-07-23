




































var bug = 352197;
var summary = 'TypeError if yield after return value in a block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'TypeError: anonymous generator function returns a value';
  
  try
  {
    var gen = eval('function() { { return 5; } yield 3; }');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
