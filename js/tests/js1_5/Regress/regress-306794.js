




































var gTestfile = 'regress-306794.js';

var BUGNUMBER = 306794;
var summary = 'Do not assert: parsing foo getter';
var actual = 'No Assertion';
var expect = 'No Assertion';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
try
{
  eval('getter\n');
}
catch(e)
{
}

reportCompare(expect, actual, summary);
