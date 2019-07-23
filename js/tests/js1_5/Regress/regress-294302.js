




































var bug = 294302;
var summary = 'JS Shell load should throw catchable exceptions';
var actual = 'Error not caught';
var expect = 'Error caught';

printBugNumber (bug);
printStatus (summary);

try
{
  load('foo.js');
}
catch(ex)
{
  actual = 'Error caught';
  printStatus(actual);
}
reportCompare(expect, actual, summary);
