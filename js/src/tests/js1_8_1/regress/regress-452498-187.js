





var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';




test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: invalid assignment to const x';
  try
  {
    eval('const x = undefined; for (x in []);');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
