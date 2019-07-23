




































var bug = 312260;
var summary = 'Switch discriminant detecting case should not warn';
var actual = 'No warning';
var expect = 'No warning';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  switch ({}.foo) {}
}
catch(e)
{
  actual = e + '';
}

jsOptions.reset();
  
reportCompare(expect, actual, summary);
