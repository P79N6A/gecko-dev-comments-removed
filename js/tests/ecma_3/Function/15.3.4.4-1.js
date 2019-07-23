















































var gTestfile = '15.3.4.4-1.js';
var UBound = 0;
var BUGNUMBER = 145791;
var summary = 'Testing ECMA conformance of Function.prototype.call';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function F0(a)
{
  return "" + this + arguments.length;
}

function F1(a)
{
  return "" + this + a;
}

function F2()
{
  return "" + this;
}






status = inSection(1);
actual = Function.prototype.call.length;
expect = 1;
addThis();






status = inSection(2);
actual = F0.call();
expect = "" + this + 0;
addThis();






status = inSection(3);
actual = F0.call("");
expect = "" + "" + 0;
addThis();

status = inSection(4);
actual = F0.call(true);
expect = "" + true + 0;
addThis();






status = inSection(5);
actual = F1.call(0, undefined);
expect = F1.call(0);
addThis();

status = inSection(6);
actual = F1.call("", undefined);
expect = F1.call("");
addThis();

status = inSection(7);
actual = F1.call(null, undefined);
expect = F1.call(null);
addThis();

status = inSection(8);
actual = F1.call(undefined, undefined);
expect = F1.call(undefined);
addThis();






status = inSection(9);
actual = F2.call(undefined);
expect = F2.call();
addThis();






status = inSection(10);
actual = F2.call(null);
expect = F2.call();
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
