


















































var gTestfile = 'regress-114491.js';
var UBound = 0;
var BUGNUMBER = 114491;
var summary = 'Regression test for bug 114491';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = 'Program execution did NOT fall into catch-block';
expect = 'Program execution fell into into catch-block';
try
{
  var sEval = 'if (true) function f(){}()';
  eval(sEval);
}
catch(e)
{
  actual = expect;
}
addThis();




test();



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
