






var gTestfile = 'regress-383682.js';

var BUGNUMBER = 383682;
var summary = 'eval is too dynamic';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(s) {
    return this.eval(s);
  }

  expect = 'PASS';
  f("function g() { return('PASS'); }");
  actual = g();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
