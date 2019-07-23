





































var gTestfile = 'regress-319391.js';

var BUGNUMBER = 319391;
var summary = 'Assignment to eval(...) should be runtime error';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var b = {};

expect = 'error';
try
{
  if (1) { eval("b.z") = 3; }
  actual = 'no error';
}
catch(ex)
{
  actual = 'error';
}
reportCompare(expect, actual, summary);

expect = 'no error';
try
{
  if (0) { eval("b.z") = 3; }
  actual = 'no error';
}
catch(ex)
{
  actual = 'error';
}
reportCompare(expect, actual, summary);
