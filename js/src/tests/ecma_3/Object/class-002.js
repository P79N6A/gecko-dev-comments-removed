


















































var gTestfile = 'class-002.js';
var i = 0;
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing the internal [[Class]] property of native constructors';
var statprefix = 'Current constructor is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];





status = 'Object';
actual = getJSClass(Object);
expect = 'Function';
addThis();

status = 'Function';
actual = getJSClass(Function);
expect = 'Function';
addThis();

status = 'Array';
actual = getJSClass(Array);
expect = 'Function';
addThis();

status = 'String';
actual = getJSClass(String);
expect = 'Function';
addThis();

status = 'Boolean';
actual = getJSClass(Boolean);
expect = 'Function';
addThis();

status = 'Number';
actual = getJSClass(Number);
expect = 'Function';
addThis();

status = 'Date';
actual = getJSClass(Date);
expect = 'Function';
addThis();

status = 'RegExp';
actual = getJSClass(RegExp);
expect = 'Function';
addThis();

status = 'Error';
actual = getJSClass(Error);
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
