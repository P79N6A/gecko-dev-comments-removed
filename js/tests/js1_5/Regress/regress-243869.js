






































var bug = 243869;
var summary = 'Rethrown custom Errors should retain file and line number';
var actual = '';
var expect = 'Test Location:123';

printBugNumber (bug);
printStatus (summary);

function bar() 
{
  try 
  {
    var f = new Error("Test Error", "Test Location", 123);
    throw f;
  }
  catch(e) 
  {
    throw e;
  }
}

try
{
  bar();
}
catch(eb)
{
  actual = eb.fileName + ':' + eb.lineNumber
}
  
reportCompare(expect, actual, summary);
