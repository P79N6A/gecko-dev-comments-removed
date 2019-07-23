




































var bug = 308556;
var summary = 'Do not treat octal sequence as regexp backrefs in strict mode';
var actual = 'No error';
var expect = 'No error';

printBugNumber (bug);
printStatus (summary);
  
options('strict');
options('werror');

try
{
  var c = eval("/\260/");
}
catch(e)
{
  actual = e + '';
}

reportCompare(expect, actual, summary);
