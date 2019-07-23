





















































var gTestfile = 'class-004.js';
var i = 0;
var UBound = 0;
var BUGNUMBER = 56868;
var summary = 'Testing the internal [[Class]] property of native error constructors';
var statprefix = 'Current constructor is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];





status = 'Error';
actual = getJSClass(Error);
expect = 'Function';
addThis();

status = 'EvalError';
actual = getJSClass(EvalError);
expect = 'Function';
addThis();

status = 'RangeError';
actual = getJSClass(RangeError);
expect = 'Function';
addThis();

status = 'ReferenceError';
actual = getJSClass(ReferenceError);
expect = 'Function';
addThis();

status = 'SyntaxError';
actual = getJSClass(SyntaxError);
expect = 'Function';
addThis();

status = 'TypeError';
actual = getJSClass(TypeError);
expect = 'Function';
addThis();

status = 'URIError';
actual = getJSClass(URIError);
expect = 'Function';
addThis();




test();




function addThis()
{
  statusList[UBound] = status;
  actualvalue[UBound] = actual;
  expectedvalue[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalue[i], actualvalue[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statprefix + statusList[i];
}
