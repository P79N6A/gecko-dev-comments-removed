















































var gTestfile = 'regress-131964.js';
var UBound = 0;
var BUGNUMBER =   131964;
var summary = 'Functions defined in global or function scope are {DontDelete}';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
function f()
{
  return 'f lives!';
}
delete f;

try
{
  actual = f();
}
catch(e)
{
  actual = 'f was deleted';
}

expect = 'f lives!';
addThis();






status = inSection(2);
function g()
{
  function f()
  {
    return 'f lives!';
  }
  delete f;

  try
  {
    actual = f();
  }
  catch(e)
  {
    actual = 'f was deleted';
  }

  expect = 'f lives!';
  addThis();
}
g();






status = inSection(3);
var s = '';
s += 'function h()';
s += '{ ';
s += '  return "h lives!";';
s += '}';
s += 'delete h;';

s += 'try';
s += '{';
s += '  actual = h();';
s += '}';
s += 'catch(e)';
s += '{';
s += '  actual = "h was deleted";';
s += '}';

s += 'expect = "h was deleted";';
s += 'addThis();';
eval(s);





status = inSection(4);
s = '';
s += 'function k()';
s += '{ ';
s += '  return "k lives!";';
s += '}';
eval(s);

delete k;

try
{
  actual = k();
}
catch(e)
{
  actual = 'k was deleted';
}

expect = 'k was deleted';
addThis();





test();




function wasDeleted(functionName)
{
  return functionName + ' was deleted...';
}


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
