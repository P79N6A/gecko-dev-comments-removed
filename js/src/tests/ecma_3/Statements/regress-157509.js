













































var gTestfile = 'regress-157509.js';
var UBound = 0;
var BUGNUMBER = 157509;
var summary = "Testing for SyntaxError on usage of '\\' in identifiers";
var TEST_PASSED = 'SyntaxError';
var TEST_FAILED = 'Generated an error, but NOT a SyntaxError!';
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
  eval('var a\\1 = 0;');
}
catch(e)
{
  if (e instanceof SyntaxError)
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
