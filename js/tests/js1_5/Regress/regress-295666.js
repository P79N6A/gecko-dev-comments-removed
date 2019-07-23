





































var bug = 295666;
var summary = 'Check JS only recursion stack overflow';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{
  throw {toString: parseInt.call};
}
catch(e)
{
}  
reportCompare(expect, actual, summary);
