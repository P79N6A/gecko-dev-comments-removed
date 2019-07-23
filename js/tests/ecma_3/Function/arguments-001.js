













































var gTestfile = 'arguments-001.js';
var UBound = 0;
var BUGNUMBER = 72884;
var summary = 'Testing the arguments object';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];
var a = '';


status = inSection(1);
function f()
{
  delete arguments.length;
  return arguments;
}

a = f();
actual = a instanceof Object;
expect = true;
addThis();

actual = a instanceof Array;
expect = false;
addThis();

actual = a.length;
expect = undefined;
addThis();



status = inSection(2);
a = f(1,2,3);
actual = a instanceof Object;
expect = true;
addThis();

actual = a instanceof Array;
expect = false;
addThis();

actual = a.length;
expect = undefined;
addThis();

actual = a[0];
expect = 1;
addThis();

actual = a[1];
expect = 2;
addThis();

actual = a[2];
expect = 3;
addThis();



status = inSection(3);





















function g()
{
  delete arguments[0];
  return arguments[0];
}
actual = g(42);
expect = undefined;  
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
