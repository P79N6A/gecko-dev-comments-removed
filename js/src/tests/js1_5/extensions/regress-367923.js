




































var gTestfile = 'regress-367923.js';

var BUGNUMBER = 367923;
var summary = 'strict warning for variable redeclares argument';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('This test will fail in Gecko prior to 1.9');

  if (!options().match(/strict/))
  {
    options('strict');
  }
  if (!options().match(/werror/))
  {
    options('werror');
  }

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
