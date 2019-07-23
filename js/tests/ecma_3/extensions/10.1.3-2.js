


















































var gTestfile = '10.1.3-2.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing functions having duplicate formal parameter names';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var OBJ = new Object();
var OBJ_TYPE = OBJ.toString();





try
{
  if (!OBJ.toSource || !uneval(OBJ))
    quit();
}
catch(e)
{
  quit();
}





function f1(x,x,x,x)
{
  var ret = eval(arguments.toSource());
  return ret.toString();
}
status = inSection(1);
actual = f1(1,2,3,4);
expect = OBJ_TYPE;
addThis();





function f2(x,x,x,x)
{
  var ret = eval(f2.arguments.toSource());
  return ret.toString();
}
status = inSection(2);
actual = f2(1,2,3,4);
expect = OBJ_TYPE;
addThis();


function f3(x,x,x,x)
{
  var ret = eval(uneval(arguments));
  return ret.toString();
}
status = inSection(3);
actual = f3(1,2,3,4);
expect = OBJ_TYPE;
addThis();





function f4(x,x,x,x)
{
  var ret = eval(uneval(f4.arguments));
  return ret.toString();
}
status = inSection(4);
actual = f4(1,2,3,4);
expect = OBJ_TYPE;
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
