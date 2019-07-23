













































var gTestfile = 'regress-154693.js';
var UBound = 0;
var BUGNUMBER = 154693;
var summary = 'Testing scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f()
{
  function nested() {}
  return nested;
}
var f1 = f();
var f2 = f();

status = inSection(1);
actual = (f1 != f2);
expect = true;
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
