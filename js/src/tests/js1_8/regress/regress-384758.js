




































var gTestfile = 'regress-384758.js';

var BUGNUMBER = 384758;
var summary = 'Statement can not follow expression closure with out intervening ;';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'SyntaxError: missing ; before statement';
  try
  {
    eval('(function() { if(t) function x() foo() bar(); })');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
