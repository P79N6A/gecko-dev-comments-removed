
























































var gTestfile = 'regress-222029-001.js';
var UBound = 0;
var BUGNUMBER = 222029;
var summary = "Make our f.caller property match IE's wrt f.apply and f.call";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f()
{
  return f.caller.p ;
}





function g()
{
  return f();
}
g.p = "hello";





function gg()
{
  return f.call(this);
}
gg.p = "hello";





function ggg()
{
  return f.apply(this);
}
ggg.p = "hello";






Function.prototype.call.p = "goodbye";
Function.prototype.apply.p = "goodbye";



status = inSection(1);
actual = g();
expect = "hello";
addThis();

status = inSection(2);
actual = gg();
expect = "hello";
addThis();

status = inSection(3);
actual = ggg();
expect = "hello";
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
