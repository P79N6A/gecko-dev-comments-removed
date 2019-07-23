




































var gTestfile = 'regress-323314-1.js';

var BUGNUMBER = 323314;
var summary = 'JSMSG_EQUAL_AS_ASSIGN in js.msg should be JSEXN_SYNTAXERR';
var actual = '';
var expect = '';

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

var xyzzy;

expect = 'SyntaxError';

try
{
  eval('if (xyzzy=1) printStatus(xyzzy);');

  actual = 'No Warning';
}
catch(ex)
{
  actual = ex.name;
  printStatus(ex + '');
}

reportCompare(expect, actual, summary);
