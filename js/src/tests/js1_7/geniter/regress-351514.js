




































var gTestfile = 'regress-351514.js';

var BUGNUMBER = 351514;
var summary = 'Finalize yield syntax to match ES4/JS2 proposal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /SyntaxError: yield expression must be parenthesized/;
  try
  {
    eval('function f() { yield g(yield 1, 2) };');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
