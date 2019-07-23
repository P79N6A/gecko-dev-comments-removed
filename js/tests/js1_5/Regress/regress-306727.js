




































var bug = 306727;
var summary = 'Parsing RegExp of octal expressions in strict mode';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();


jsOptions.setOption('strict', false);
jsOptions.setOption('werror', false);
try
{
  expect = null;
  actual = /.\011/.exec ('a'+String.fromCharCode(0)+'11');
}
catch(e)
{
}
jsOptions.reset();
reportCompare(expect, actual, summary);


jsOptions.setOption('strict', true);
expect = null;
try
{
  actual = /.\011/.exec ('a'+String.fromCharCode(0)+'11');
}
catch(e)
{
}
jsOptions.reset();
reportCompare(expect, actual, summary);

  
