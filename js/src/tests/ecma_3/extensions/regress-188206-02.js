






































var gTestfile = 'regress-188206-02.js';
var UBound = 0;
var BUGNUMBER = 188206;
var summary = 'Invalid use of regexp quantifiers should generate SyntaxErrors';
var CHECK_PASSED = 'Should not generate an error';
var CHECK_FAILED = 'Generated an error!';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


















status = inSection(13);
checkThis(' /a*{/ ');

status = inSection(14);
checkThis(' /a{}/ ');

status = inSection(15);
checkThis(' /{a/ ');

status = inSection(16);
checkThis(' /}a/ ');

status = inSection(17);
checkThis(' /x{abc}/ ');

status = inSection(18);
checkThis(' /{{0}/ ');

status = inSection(19);
checkThis(' /{{1}/ ');

status = inSection(20);
checkThis(' /x{{0}/ ');

status = inSection(21);
checkThis(' /x{{1}/ ');

status = inSection(22);
checkThis(' /x{{0}}/ ');

status = inSection(23);
checkThis(' /x{{1}}/ ');

status = inSection(24);
checkThis(' /x{{0}}/ ');

status = inSection(25);
checkThis(' /x{{1}}/ ');

status = inSection(26);
checkThis(' /x{{0}}/ ');

status = inSection(27);
checkThis(' /x{{1}}/ ');



test();







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
