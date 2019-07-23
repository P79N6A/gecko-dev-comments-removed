
























































var gTestfile = 'regress-222029-002.js';
var UBound = 0;
var BUGNUMBER = 222029;
var summary = "Make our f.caller property match IE's wrt f.apply and f.call";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];




var p = 'global';
var o = {p:'object'};


function f(obj)
{
  return f.caller.p ;
}





function g(obj)
{
  var p = 'local';
  return f(obj);
}
g.p = "hello";





function gg(obj)
{
  var p = 'local';
  return f.call(obj, obj);
}
gg.p = "hello";





function ggg(obj)
{
  var p = 'local';
  return f.apply(obj, [obj]);
}
ggg.p = "hello";






Function.prototype.call.p = "goodbye";
Function.prototype.apply.p = "goodbye";



status = inSection(1);
actual = g(o);
expect = "hello";
addThis();

status = inSection(2);
actual = gg(o);
expect = "hello";
addThis();

status = inSection(3);
actual = ggg(o);
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
