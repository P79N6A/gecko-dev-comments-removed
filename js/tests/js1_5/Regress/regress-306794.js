




































var bug = 306794;
var summary = 'Assertion parsing foo getter';
var actual = 'No Assertion';
var expect = 'No Assertion';

printBugNumber (bug);
printStatus (summary);
  
try
{
  eval('getter\n');
}
catch(e)
{
}

reportCompare(expect, actual, summary);
