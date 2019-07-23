

























































var gTestfile = 'regress-131348.js';
var UBound = 0;
var BUGNUMBER = 131348;
var summary = 'JS should not error on |for(i in undefined)|, |for(i in null)|';
var TEST_PASSED = 'No error';
var TEST_FAILED = 'An error was generated!!!';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



status = inSection(1);
expect = TEST_PASSED;
actual = TEST_PASSED;
try
{
  for (var i in undefined)
  {
    print(i);
  }
}
catch(e)
{
  actual = TEST_FAILED;
}
addThis();



status = inSection(2);
expect = TEST_PASSED;
actual = TEST_PASSED;
try
{
  for (var i in null)
  {
    print(i);
  }
}
catch(e)
{
  actual = TEST_FAILED;
}
addThis();



status = inSection(3);
expect = TEST_PASSED;
actual = TEST_PASSED;






try
{
  for (var i in this.ZZZ)
  {
    print(i);
  }
}
catch(e)
{
  actual = TEST_FAILED;
}
addThis();



status = inSection(4);
expect = TEST_PASSED;
actual = TEST_PASSED;




try
{
  for (var i in 'bbb'.match(/aaa/))
  {
    print(i);
  }
}
catch(e)
{
  actual = TEST_FAILED;
}
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
