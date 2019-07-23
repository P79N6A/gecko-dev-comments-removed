















































var gTestfile = '15.3.4.3-1.js';
var UBound = 0;
var BUGNUMBER = 145791;
var summary = 'Testing ECMA conformance of Function.prototype.apply';
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
actual = Function.prototype.apply.length;
expect = 2;
addThis();






status = inSection(2);
actual = F0.apply();
expect = "" + this + 0;
addThis();






status = inSection(3);
actual = F0.apply("");
expect = "" + "" + 0;
addThis();

status = inSection(4);
actual = F0.apply(true);
expect = "" + true + 0;
addThis();






status = inSection(5);
actual = F1.apply(0, undefined);
expect = F1.apply(0);
addThis();

status = inSection(6);
actual = F1.apply("", undefined);
expect = F1.apply("");
addThis();

status = inSection(7);
actual = F1.apply(null, undefined);
expect = F1.apply(null);
addThis();

status = inSection(8);
actual = F1.apply(undefined, undefined);
expect = F1.apply(undefined);
addThis();






status = inSection(9);
actual = F1.apply(0, null);
expect = F1.apply(0);
addThis();

status = inSection(10);
actual = F1.apply("", null);
expect = F1.apply("");
addThis();

status = inSection(11);
actual = F1.apply(null, null);
expect = F1.apply(null);
addThis();

status = inSection(12);
actual = F1.apply(undefined, null);
expect = F1.apply(undefined);
addThis();






status = inSection(13);
actual = F2.apply(undefined);
expect = F2.apply();
addThis();






status = inSection(14);
actual = F2.apply(null);
expect = F2.apply();
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
