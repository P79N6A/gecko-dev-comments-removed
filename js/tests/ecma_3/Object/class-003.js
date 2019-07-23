





















































var gTestfile = 'class-003.js';
var i = 0;
var UBound = 0;
var BUGNUMBER = 56868;
var summary = 'Testing the internal [[Class]] property of native error types';
var statprefix = 'Current object is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];





status = 'new Error()';
actual = getJSClass(new Error());
expect = 'Error';
addThis();

status = 'new EvalError()';
actual = getJSClass(new EvalError());
expect = 'Error';
addThis();

status = 'new RangeError()';
actual = getJSClass(new RangeError());
expect = 'Error';
addThis();

status = 'new ReferenceError()';
actual = getJSClass(new ReferenceError());
expect = 'Error';
addThis();

status = 'new SyntaxError()';
actual = getJSClass(new SyntaxError());
expect = 'Error';
addThis();

status = 'new TypeError()';
actual = getJSClass(new TypeError());
expect = 'Error';
addThis();

status = 'new URIError()';
actual = getJSClass(new URIError());
expect = 'Error';
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
