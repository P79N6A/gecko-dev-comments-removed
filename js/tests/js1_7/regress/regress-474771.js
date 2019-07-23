




































var gTestfile = 'regress-474771.js';

var BUGNUMBER = 474771;
var summary = 'TM: do not halt execution with gczeal, prototype mangling, for..in';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'PASS';
  jit(true);

  if (typeof gczeal != 'undefined')
  {
    gczeal(2);
  }
  Object.prototype.q = 3;
  for each (let x in [6, 7]) { } print(actual = "PASS");
 
  jit(false);

  delete Object.prototype.q;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
