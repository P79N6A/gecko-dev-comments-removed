





































var bug = 347559;
var summary = 'Let declarations should not warn that function does not ' +
  'return a value';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var jsOptions = new JavaScriptOptions();

  actual = 'No Warning';
  expect = 'No Warning';

  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);

  try
  {
    eval('function f() { let a = 2; return a; }');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  jsOptions.reset();
  
  reportCompare(expect, actual, summary + ': 1');

  actual = 'No Warning';
  expect = 'TypeError: function f does not always return a value';

  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);

  try
  {
    eval('function f() { if (asdf) return 3; let x; }');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  jsOptions.reset();
  
  reportCompare(expect, actual, summary + ': 2');

  actual = 'No Warning';
  expect = 'No Warning';

  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);

  try
  {
    eval('function f() { if (asdf) return 3; let (x) { return 4; } }');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  jsOptions.reset();
  
  reportCompare(expect, actual, summary + ': 3');

  exitFunc ('test');
}
