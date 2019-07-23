




































var gTestfile = 'regress-452498-181.js';

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

  expect = 3;

  (function(print) { delete print; })(); print(actual = 3);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
