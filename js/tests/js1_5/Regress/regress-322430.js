




































var bug = 322430;
var summary = 'Remove deprecated with statement warning';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var jsOptions = new JavaScriptOptions();
jsOptions.setOption('strict', true);
jsOptions.setOption('werror', true);

expect = 'No Warning';

try
{
  var obj = {foo: 'baz'};
  
  
  
  
  
  eval('with (obj) { foo; }');

  actual = 'No Warning';
}
catch(ex)
{
  actual = ex + '';
}
jsOptions.reset();

reportCompare(expect, actual, summary);
