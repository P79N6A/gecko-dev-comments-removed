





































var BUGNUMBER = 295052;
var summary = 'Do not crash when apply method is called on String.prototype.match';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  "".match.apply();
  throw new Error("should have thrown for undefined this");
}
catch (e)
{
  assertEq(e instanceof TypeError, true,
           "No TypeError for String.prototype.match");
}
 
reportCompare(expect, actual, summary);
