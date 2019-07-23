




































var gTestfile = 'regress-452498-107.js';

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





  function d() { const d; ++d; }
  expect  = 'function d() { const d; + d + 1; }';
  actual = d + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
