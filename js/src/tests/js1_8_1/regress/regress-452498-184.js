




































var gTestfile = 'regress-452498-184.js';

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

  expect = '11';

  const e = 8; print(actual = '' + ((e += 3)));

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
