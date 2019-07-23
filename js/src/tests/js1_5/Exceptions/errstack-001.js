




























































var gTestfile = 'errstack-001.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing Error.stack';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var myErr = '';
var stackFrames = '';


function A(x,y)
{
  return B(x+1,y+1);
}

function B(x,z)
{
  return C(x+1,z+1);
}

function C(x,y)
{
  return D(x+1,y+1);
}

function D(x,z)
{
  try
  {
    throw new Error('meep!');
  }
  catch (e)
  {
    return e;
  }
}


myErr = A(44,13);
stackFrames = getStackFrames(myErr);
status = inSection(1);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();

status = inSection(2);
actual = stackFrames[1].substring(0,9);
expect = 'A(44,13)@';
addThis();

status = inSection(3);
actual = stackFrames[2].substring(0,9);
expect = 'B(45,14)@';
addThis();

status = inSection(4);
actual = stackFrames[3].substring(0,9);
expect = 'C(46,15)@';
addThis();

status = inSection(5);
actual = stackFrames[4].substring(0,9);
expect = 'D(47,16)@';
addThis();



myErr = A('44:foo','13:bar');
stackFrames = getStackFrames(myErr);
status = inSection(6);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();

status = inSection(7);
actual = stackFrames[1].substring(0,21);
expect = 'A("44:foo","13:bar")@';
addThis();

status = inSection(8);
actual = stackFrames[2].substring(0,23);
expect = 'B("44:foo1","13:bar1")@';
addThis();

status = inSection(9);
actual = stackFrames[3].substring(0,25);
expect = 'C("44:foo11","13:bar11")@';
addThis();

status = inSection(10);
actual = stackFrames[4].substring(0,27);
expect = 'D("44:foo111","13:bar111")@';;
addThis();






myErr = function() { return A(44,13); } ();
stackFrames = getStackFrames(myErr);
status = inSection(11);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();

status = inSection(12);
actual = stackFrames[1].substring(0,3);
expect = '()@';
addThis();

status = inSection(13);
actual = stackFrames[2].substring(0,9);
expect = 'A(44,13)@';
addThis();








var f = Function('return A(44,13);');
myErr = f();
stackFrames = getStackFrames(myErr);
status = inSection(14);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();

status = inSection(15);
actual = stackFrames[1].substring(0,12);
expect = 'anonymous()@';
addThis();

status = inSection(16);
actual = stackFrames[2].substring(0,9);
expect = 'A(44,13)@';
addThis();








var message = 'Hi there!'; var fileName = 'file name'; var lineNumber = 0;
myErr = Error(message, fileName, lineNumber);
stackFrames = getStackFrames(myErr);
status = inSection(17);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();





myErr = new Error(message, fileName, lineNumber);
stackFrames = getStackFrames(myErr);
status = inSection(18);
actual = stackFrames[0].substring(0,1);
expect = '@';
addThis();





test();












function getStackFrames(err)
{
  var arr = err.stack.split('\n');
  arr.pop();
  return arr.reverse();
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
