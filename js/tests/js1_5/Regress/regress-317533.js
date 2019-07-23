




































var bug = 317533;
var summary = 'improve function does not always return a value warnings';
var actual = '';
var expect = 'TypeError: anonymous function does not always return a value';

printBugNumber (bug);
printStatus (summary);
  
var code;
var jsOptions = new JavaScriptOptions();

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  actual = ''; 
  code = "function(x){ if(x) return x; }";
  printStatus(code);
  eval(code);
}
catch(ex)
{
  actual = ex + '';
  printStatus(ex);
}
jsOptions.reset();
reportCompare(expect, actual, summary);

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  actual = '';
  code = "function(x){ if(x) return x; ;}";
  printStatus(code);
  eval(code);
}
catch(ex)
{
  actual = ex + '';
  printStatus(ex);
}
jsOptions.reset();
reportCompare(expect, actual, summary);
