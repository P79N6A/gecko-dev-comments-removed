













































var gTestfile = '8.6.2.6-001.js';
var UBound = 0;
var BUGNUMBER = 167325;
var summary = "Test for TypeError on invalid default string value of object";
var TEST_PASSED = 'TypeError';
var TEST_FAILED = 'Generated an error, but NOT a TypeError!';
var TEST_FAILED_BADLY = 'Did not generate ANY error!!!';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
expect = TEST_PASSED;
actual = TEST_FAILED_BADLY;




try
{
  var obj = {toString: function() {return new Object();}}
  obj == 'abc';
}
catch(e)
{
  if (e instanceof TypeError)
    actual = TEST_PASSED;
  else
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
