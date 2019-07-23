





































gTestfile = 'regress-99663.js';


var UBound = 0;
var BUGNUMBER = 99663;
var summary = 'Regression test for Bugzilla bug 99663';




var READONLY = /read\s*-?\s*only/;
var READONLY_TRUE = 'a "read-only" error';
var READONLY_FALSE = 'Error: ';
var FAILURE = 'NO ERROR WAS GENERATED!';
var status = '';
var actual = '';
var expect= '';
var statusitems = [];
var expectedvalues = [];
var actualvalues = [];





function f1()
{
  with (it)
  {
    for (rdonly in this);
  }
}


function f2()
{
  for (it.rdonly in this);
}


function f3(s)
{
  for (it[s] in this);
}







actual = FAILURE;
try
{
  f1();
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 1 of test - got ' + actual;
addThis();


actual = FAILURE;
try
{
  f2();
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 2 of test - got ' + actual;
addThis();


actual = FAILURE;
try
{
  f3('rdonly');
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 3 of test - got ' + actual;
addThis();




test();




function readOnly(msg)
{
  if (msg.match(READONLY))
    return READONLY_TRUE;
  return READONLY_FALSE + msg;
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
  print ('Bug Number ' + bug);
  print ('STATUS: ' + summary);

  for (var i=0; i<UBound; i++)
  {
    writeTestCaseResult(expectedvalues[i], actualvalues[i], statusitems[i]);
  }
}
