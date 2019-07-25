






var BUGNUMBER = 618572;
var summary = 'Do not assert when ungetting a Unicode char sequence';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'SyntaxError: illegal character';

  try
  {
    eval("var a\\u0346 = 3;");
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
