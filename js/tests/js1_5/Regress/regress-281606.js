





































var gTestfile = 'regress-281606.js';

var BUGNUMBER = 281606;
var summary = 'l instanceof r throws TypeError if r does not support [[HasInstance]]';
var actual = '';
var expect = '';
var status;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var o = {};

status = summary + ' ' + inSection(1) + ' o instanceof Math ';
expect = 'TypeError';
try
{
  if (o instanceof Math)
  {
  }
  actual = 'No Error';
}
catch(e)
{
  actual = e.name;
}
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + ' o instanceof o ';
expect = 'TypeError';
try
{
  if (o instanceof o)
  {
  }
  actual = 'No Error';
}
catch(e)
{
  actual = e.name;
}
reportCompare(expect, actual, status);
