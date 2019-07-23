




































var bug = 352197;
var summary = 'Strict warning for return e; vs. return;';
var actual = '';
var expect = 'TypeError: function f does not always return a value';

printBugNumber (bug);
printStatus (summary);
  
var jsOptions = new JavaScriptOptions();

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  eval('function f() { if (x) return y; }');
}
catch(ex)
{
  actual = ex + '';
}

jsOptions.reset(); 

reportCompare(expect, actual, summary);

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

try
{
  eval('function f() { if (x) { return y; } }');
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset(); 

reportCompare(expect, actual, summary);
