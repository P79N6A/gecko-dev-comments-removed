




































var gTestfile = '15.7.4.5-2.js';

var BUGNUMBER = 469397;
var summary = '(0.5).toFixed(0) == 1';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '1';
  actual = (0.5).toFixed(0);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
