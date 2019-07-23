




































var gTestfile = 'regress-452498-039.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = '10';



  const e = 0;
  print(actual = ++e);

  actual = String(actual) + e;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
