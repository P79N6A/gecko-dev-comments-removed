




































var bug = 106244;
var summary = 'No warning in strict mode if (a = b && c == d)...';
var actual = '';
var expect = 'test for equality (==) mistyped as assignment (=)?';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

var a = false;
var b = true;
var c = false;
var d = true;
var result;

try
{
  if (a = b && c == d) 
    result = true;
  else 
    result = false;
}
catch(ex)
{
  actual = ex.message;
}
jsOptions.reset(); 

printStatus('result = ' + result); 
reportCompare(expect, actual, summary);
