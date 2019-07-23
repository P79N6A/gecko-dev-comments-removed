




































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

  options('strict');
  options('werror');

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
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
