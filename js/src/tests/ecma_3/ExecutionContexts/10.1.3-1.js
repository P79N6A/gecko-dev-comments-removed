
















































var gTestfile = '10.1.3-1.js';
var UBound = 0;
var BUGNUMBER = 124900;
var summary = 'Testing functions having duplicate formal parameter names';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f1(x,x)
{
  return x;
}
status = inSection(1);
actual = f1(1,2);
expect = 2;
addThis();


function f2(x,x,x)
{
  return x*x*x;
}
status = inSection(2);
actual = f2(1,2,3);
expect = 27;
addThis();


function f3(x,x,x,x)
{
  return 'a' + x + 'b' + x + 'c' + x ;
}
status = inSection(3);
actual = f3(1,2,3,4);
expect = 'a4b4c4';
addThis();






function f4(x,a,b,x,z)
{
  return x;
}
status = inSection(4);
actual = f4(1,2);
expect = undefined;
addThis();





function f5(x,x,x,x)
{
}
status = inSection(5);
actual = f5.toString().match(/\((.*)\)/)[1];
actual = actual.replace(/\s/g, ''); 
expect = 'x,x,x,x';
addThis();


function f6(x,x,x,x)
{
  var ret = [];

  for (var i=0; i<arguments.length; i++)
    ret.push(arguments[i]);

  return ret.toString();
}
status = inSection(6);
actual = f6(1,2,3,4);
expect = '1,2,3,4';
addThis();






function f7(x,x,x,x)
{
  x = 999;
  var ret = [];

  for (var i=0; i<arguments.length; i++)
    ret.push(arguments[i]);

  return ret.toString();
}
status = inSection(7);
actual = f7(1,2,3,4);
expect = '1,2,3,999';
addThis();





function f8(x,x,x,x)
{
  var x = 999;
  var ret = [];

  for (var i=0; i<arguments.length; i++)
    ret.push(arguments[i]);

  return ret.toString();
}
status = inSection(8);
actual = f8(1,2,3,4);
expect = '1,2,3,999';
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
