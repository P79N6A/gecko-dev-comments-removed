




































var gTestfile = 'regress-459606.js';

var BUGNUMBER = 459606;
var summary = '((0.1).toFixed()).toSource()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '(new String("0"))';
  actual = ((0.1).toFixed()).toSource();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
