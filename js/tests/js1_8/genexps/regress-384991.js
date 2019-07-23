




































var gTestfile = 'regress-384991.js';

var BUGNUMBER = 384991;
var summary = ' w(yield) should not cause "yield expression must be parenthesized" syntax error';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'No Error';

  try
  {
    actual = 'No Error';
    (function() { w((yield)); });
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': 1');

  try
  {
    actual = 'No Error';
    (function() { w(1 ? yield : 0); });
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': 2');

  try
  {
    actual = 'No Error';
    (function () { f(x = yield); const x; });
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': 3');

  exitFunc ('test');
}
