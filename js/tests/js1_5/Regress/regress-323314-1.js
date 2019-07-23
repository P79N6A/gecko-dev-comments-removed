




































var bug = 323314;
var summary = 'JSMSG_EQUAL_AS_ASSIGN in js.msg should be JSEXN_SYNTAXERR';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

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
jsOptions.reset();

reportCompare(expect, actual, summary);
