




































var gTestfile = 'regress-352197.js';

var BUGNUMBER = 352197;
var summary = 'Strict warning for return e; vs. return;';
var actual = '';
var expect = 'TypeError: function f does not always return a value';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
options('strict');
options('werror');

try
{
  eval('function f() { if (x) return y; }');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary);

try
{
  eval('function f() { if (x) { return y; } }');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary);
