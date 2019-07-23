




































var gTestfile = 'regress-315147.js';

var BUGNUMBER = 315147;
var summary = 'Error JSMSG_UNDEFINED_PROP should be JSEXN_REFERENCEERR';
var actual = '';
var expect = 'ReferenceError';

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

var o = {};

try
{
  o.foo;
  actual = 'no error';
}
catch(ex)
{
  actual = ex.name;
}
 
reportCompare(expect, actual, summary);
