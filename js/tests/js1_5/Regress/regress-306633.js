




































var bug = 306727;
var summary = 'report compile warnings in evald code when strict warnings enabled';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();


jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

expect = 'SyntaxError';

try
{
  actual = eval('super = 5');
}
catch(e)
{
  actual = e.name;
}
jsOptions.reset();

reportCompare(expect, actual, summary);

  
