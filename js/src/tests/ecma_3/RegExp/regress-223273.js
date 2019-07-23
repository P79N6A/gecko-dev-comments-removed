



























































var gTestfile = 'regress-223273.js';
var UBound = 0;
var BUGNUMBER = 223273;
var summary = 'Unescaped, unbalanced parens in regexp should be a SyntaxError';
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
testThis(' /(/ ');

status = inSection(2);
testThis(' /)/ ');

status = inSection(3);
testThis(' /(abc\\)def(g/ ');

status = inSection(4);
testThis(' /\\(abc)def)g/ ');






status = inSection(5);
checkThis(' /\\(/ ');

status = inSection(6);
checkThis(' /\\)/ ');

status = inSection(7);
checkThis(' /(abc)def\\(g/ ');

status = inSection(8);
checkThis(' /(abc\\)def)g/ ');

status = inSection(9);
checkThis(' /(abc(\\))def)g/ ');

status = inSection(10);
checkThis(' /(abc([x\\)yz]+)def)g/ ');






status = inSection(11);
testThis(' /[/ ');

status = inSection(12);
testThis(' /[abc\\]def[g/ ');







status = inSection(13);
checkThis(' /]/ ');

status = inSection(14);
checkThis(' /\\[abc]def]g/ ');






status = inSection(15);
checkThis(' /\\[/ ');

status = inSection(16);
checkThis(' /\\]/ ');

status = inSection(17);
checkThis(' /[abc]def\\[g/ ');

status = inSection(18);
checkThis(' /[abc\\]def]g/ ');

status = inSection(19);
checkThis(' /(abc[\\]]def)g/ ');

status = inSection(20);
checkThis(' /[abc(x\\]yz+)def]g/ ');










status = inSection(21);
checkThis(' /abc{def/ ');

status = inSection(22);
checkThis(' /abc}def/ ');

status = inSection(23);
checkThis(' /a{2}bc{def/ ');

status = inSection(24);
checkThis(' /a}b{3}c}def/ ');






status = inSection(25);
checkThis(' /abc\\{def/ ');

status = inSection(26);
checkThis(' /abc\\}def/ ');

status = inSection(27);
checkThis(' /a{2}bc\\{def/ ');

status = inSection(28);
checkThis(' /a\\}b{3}c\\}def/ ');





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





function checkThis(sValidSyntax)
{
  expect = CHECK_PASSED;
  actual = CHECK_PASSED;

  try
  {
    eval(sValidSyntax);
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
