




































var gTestfile = 'regress-452498-077.js';

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

  expect = '2';



  (function () { const [d] = [1]; [d] = [2]; print(actual = d);})();

  actual = String(actual);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
