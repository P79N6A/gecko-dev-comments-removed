














































var gTestfile = 'regress-137181.js';
var UBound = 0;
var BUGNUMBER = 137181;
var summary = 'delete arguments[i] should break connection to local reference';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
function f1(x)
{
  x = 1;
  delete arguments[0];
  return x;
}
actual = f1(0); 
expect = 1;
addThis();


status = inSection(2);
function f2(x)
{
  x = 1;
  delete arguments[0];
  arguments[0] = -1;
  return x;
}
actual = f2(0); 
expect = 1;
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
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
