




































var gTestfile = 'regress-452498-108.js';

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



  function p(){p}

  expect = 'function p(){p;}';
  actual = p + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
