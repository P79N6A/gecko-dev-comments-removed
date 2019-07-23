




































var gTestfile = 'regress-474771.js';

var BUGNUMBER = 474771;
var summary = 'gczeal, prototype mangling, for..in';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'PASS';
  if (typeof gczeal == 'function')
  {
    gczeal(2);
  }

  Object.prototype.q = 3;
  for each (let x in [6, 7]) { } print(actual = "PASS");

  if (typeof gczeal == 'function')
  {
    gczeal(0);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
