




































var gTestfile = 'regress-334807-06.js';

var BUGNUMBER = 334807;
var summary = '12.14 - exception prototype is the original Object prototype.';
var actual = 'No Error';
var expect = 'ReferenceError';

printBugNumber(BUGNUMBER);
printStatus (summary);

printStatus('set Error = Number');

Error = Number;

try
{
  x.y;
}
catch(ex)
{
  try
  {
    actual = ex.name;
    printStatus(ex + ': x.y');
    ex.valueOf();
  }
  catch(ex2)
  {
    printStatus(ex2 + ': ex.valueOf()');
    actual = ex2.name;
  }
}
reportCompare(expect, actual, summary);
