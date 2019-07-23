




































var bug = 367923;
var summary = 'strict warning for variable redeclares argument';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  print('This test will fail in Gecko prior to 1.9');

  var jsOptions = new JavaScriptOptions();


  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);
  try
  {
    expect = 'TypeError: variable v redeclares argument';
    
    eval("(function (v) { var v; })(1)");
    actual = 'No warning';
  }
  catch(ex)
  {
    actual = ex + '';
    print(ex);
  }
  jsOptions.reset();
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
