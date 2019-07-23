




































var bug = 352197;
var summary = 'Strict warning for return e; vs. return;';
var actual = '';
var expect = 'TypeError: function f does not always return a value';

printBugNumber (bug);
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
