




































var bug = 355002;
var summary = 'Do not assert on |for each (this in []) { }|';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 'SyntaxError: invalid for/in left-hand side';
  actual = '';
  try
  {  
    eval('for each (this in []) { }');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
