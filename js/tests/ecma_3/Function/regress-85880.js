

















































var gTestfile = 'regress-85880.js';
var UBound = 0;
var BUGNUMBER = 85880;
var summary = 'Arguments object of g(){f()} should not be null';
var cnNonNull = 'Arguments != null';
var cnNull = 'Arguments == null';
var cnRecurse = true;
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f1(x)
{
}


function f2()
{
  return f2.arguments;
}
status = 'Section A of test';
actual = (f2() == null);
expect = false;
addThis();

status = 'Section B of test';
actual = (f2(0) == null);
expect = false;
addThis();


function f3()
{
  f1();
  return f3.arguments;
}
status = 'Section C of test';
actual = (f3() == null);
expect = false;
addThis();

status = 'Section D of test';
actual = (f3(0) == null);
expect = false;
addThis();


function f4()
{
  f1();
  f2();
  f3();
  return f4.arguments;
}
status = 'Section E of test';
actual = (f4() == null);
expect = false;
addThis();

status = 'Section F of test';
actual = (f4(0) == null);
expect = false;
addThis();


function f5()
{
  if (cnRecurse)
  {
    cnRecurse = false;
    f5();
  }
  return f5.arguments;
}
status = 'Section G of test';
actual = (f5() == null);
expect = false;
addThis();

status = 'Section H of test';
actual = (f5(0) == null);
expect = false;
addThis();




test();



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = isThisNull(actual);
  expectedvalues[UBound] = isThisNull(expect);
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function isThisNull(bool)
{
  return bool? cnNull : cnNonNull
    }
