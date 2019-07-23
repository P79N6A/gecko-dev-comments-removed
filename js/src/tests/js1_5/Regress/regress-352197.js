




































var gTestfile = 'regress-352197.js';

var BUGNUMBER = 352197;
var summary = 'Strict warning for return e; vs. return;';
var actual = '';
var expect = 'TypeError: function f does not always return a value';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
if (!options().match(/strict/))
{
  options('strict');
}
if (!options().match(/werror/))
{
  options('werror');
}

try
{
  eval('function f() { if (x) return y; }');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary + ': 1');

try
{
  eval('function f() { if (x) { return y; } }');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary + ': 2');

var f;
expect = 'TypeError: function anonymous does not always return a value';

try
{
  f = Function('if (x) return y;');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary + ': 3');

try
{
  f = Function('if (x) { return y; }');
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary + ': 4');
