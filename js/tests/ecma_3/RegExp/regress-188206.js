
















































var gTestfile = 'regress-188206.js';
var UBound = 0;
var BUGNUMBER = 188206;
var summary = 'Invalid use of regexp quantifiers should generate SyntaxErrors';
var TEST_PASSED = 'SyntaxError';
var TEST_FAILED = 'Generated an error, but NOT a SyntaxError!';
var TEST_FAILED_BADLY = 'Did not generate ANY error!!!';
var CHECK_PASSED = 'Should not generate an error';
var CHECK_FAILED = 'Generated an error!';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];










status = inSection(1);
testThis(' /a**/ ');

status = inSection(2);
testThis(' /a***/ ');

status = inSection(3);
testThis(' /a++/ ');

status = inSection(4);
testThis(' /a+++/ ');






status = inSection(5);
testThis(' /a???/ ');

status = inSection(6);
testThis(' /a????/ ');





status = inSection(9);
testThis(' /+a/ ');

status = inSection(10);
testThis(' /++a/ ');

status = inSection(11);
testThis(' /?a/ ');

status = inSection(12);
testThis(' /??a/ ');








status = inSection(28);
testThis(' /x{1}{1}/ ');

status = inSection(29);
testThis(' /x{1,}{1}/ ');

status = inSection(30);
testThis(' /x{1,2}{1}/ ');

status = inSection(31);
testThis(' /x{1}{1,}/ ');

status = inSection(32);
testThis(' /x{1,}{1,}/ ');

status = inSection(33);
testThis(' /x{1,2}{1,}/ ');

status = inSection(34);
testThis(' /x{1}{1,2}/ ');

status = inSection(35);
testThis(' /x{1,}{1,2}/ ');

status = inSection(36);
testThis(' /x{1,2}{1,2}/ ');




test();







function testThis(sInvalidSyntax)
{
  expect = TEST_PASSED;
  actual = TEST_FAILED_BADLY;

  try
  {
    eval(sInvalidSyntax);
  }
  catch(e)
  {
    if (e instanceof SyntaxError)
      actual = TEST_PASSED;
    else
      actual = TEST_FAILED;
  }

  statusitems[UBound] = status;
  expectedvalues[UBound] = expect;
  actualvalues[UBound] = actual;
  UBound++;
}





function checkThis(sAllowedSyntax)
{
  expect = CHECK_PASSED;
  actual = CHECK_PASSED;

  try
  {
    eval(sAllowedSyntax);
  }
  catch(e)
  {
    actual = CHECK_FAILED;
  }

  statusitems[UBound] = status;
  expectedvalues[UBound] = expect;
  actualvalues[UBound] = actual;
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
