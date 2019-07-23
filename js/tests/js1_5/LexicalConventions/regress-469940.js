




































var gTestfile = 'regress-469940.js';

var BUGNUMBER = 469940;
var summary = 'Do not insert semi-colon after var with multiline initializer';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'SyntaxError: missing ; before statement';

  var s = 'var x = function f() { \n return 42; } print(x);';

  try
  {
    eval(s);
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
