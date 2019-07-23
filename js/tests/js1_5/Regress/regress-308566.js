




































var bug = 308556;
var summary = 'Do not treat octal sequence as regexp backrefs in strict mode';
var actual = 'No error';
var expect = 'No error';

printBugNumber (bug);
printStatus (summary);
  
var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  var c = eval("/\260/");
}
catch(e)
{
  actual = e + '';
}

jsOptions.reset();

reportCompare(expect, actual, summary);
