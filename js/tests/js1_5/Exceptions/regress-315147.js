




































var bug = 315147;
var summary = 'Error JSMSG_UNDEFINED_PROP should be JSEXN_REFERENCEERR';
var actual = '';
var expect = 'ReferenceError';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();


jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

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
jsOptions.reset();
  
reportCompare(expect, actual, summary);
