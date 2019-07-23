













































var gTestfile = 'regress-220362.js';
var UBound = 0;
var BUGNUMBER = 220362;
var summary = 'Calling a local function from global scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



function a()
{
  var x = 'A';
  var f = function() {return x;};
  return f();
}


function b()
{
  var x = 'B';
  var f = function() {return x;};
  return f;
}


status = inSection(1);
actual = a();
expect = 'A';
addThis();

status = inSection(2);
var f = b();
actual = f();
expect = 'B';
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
