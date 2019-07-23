




































var gTestfile = 'regress-474769.js';

var BUGNUMBER = 474769;
var summary = 'TM: nested for each type-unstable loops';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 1;

  jit(true);

  for each (b in [1, 1, 1, 1.5, 1, 1]) {
      (function() { for each (let h in [0, 0, 1.4, ""]) {} })();
  }
  actual = b;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
