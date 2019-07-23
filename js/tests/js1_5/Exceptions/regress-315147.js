




































var bug = 315147;
var summary = 'Error JSMSG_UNDEFINED_PROP should be JSEXN_REFERENCEERR';
var actual = '';
var expect = 'ReferenceError';

printBugNumber (bug);
printStatus (summary);

options('strict');
options('werror');

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
