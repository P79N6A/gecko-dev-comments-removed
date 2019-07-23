




































var bug = 253150;
var summary = 'Do not warn on detecting properties';
var actual = '';
var expect = 'No warning';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();

var testobject = {};

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  var testresult = testobject.foo;
  actual = 'No warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();  
reportCompare(expect, actual, summary + ': 1');

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  if (testobject.foo)
  {
    ;
  }
  actual = 'No warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();  
reportCompare(expect, actual, summary + ': 2');

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  if (typeof testobject.foo == 'undefined')
  {
    ;
  }
  actual = 'No warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();  
reportCompare(expect, actual, summary + ': 3');

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  if (testobject.foo == null)
  {
    ;
  }
  actual = 'No warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();  
reportCompare(expect, actual, summary + ': 4');

jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);
try
{
  if (testobject.foo == undefined)
  {
    ;
  }
  actual = 'No warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();  
reportCompare(expect, actual, summary + ': 3');
