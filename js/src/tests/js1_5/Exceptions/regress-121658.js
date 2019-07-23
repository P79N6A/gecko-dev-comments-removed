

















































var gTestfile = 'regress-121658.js';
var UBound = 0;
var BUGNUMBER = 121658;
var msg = '"Too much recursion" errors should be safely caught by try...catch';
var TEST_PASSED = 'i retained the value it had at location of error';
var TEST_FAILED = 'i did NOT retain this value';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var i;


function f()
{
  ++i;

  
  try
  {
    f();
  }
  catch(e)
  {
  }
}

i=0;
f();
status = inSection(1);
actual = (i>0);
expect = true;
addThis();




function g()
{
  f();
}

i=0;
g();
status = inSection(2);
actual = (i>0);
expect = true;
addThis();




var sEval = 'function h(){++i; try{h();} catch(e){}}; i=0; h();';
eval(sEval);
status = inSection(3);
actual = (i>0);
expect = true;
addThis();




sEval = 'function a(){++i; try{h();} catch(e){}}; i=0; a();';
eval(sEval);
status = inSection(4);
actual = (i>0);
expect = true;
addThis();





test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = formatThis(actual);
  expectedvalues[UBound] = formatThis(expect);
  UBound++;
}


function formatThis(bool)
{
  return bool? TEST_PASSED : TEST_FAILED;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(msg);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
