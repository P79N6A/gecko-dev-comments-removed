




































var gTestfile = 'regress-416354.js';

var BUGNUMBER = 416354;
var summary = 'GC hazard due to missing SAVE_SP_AND_PC';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f(a, b, c)
  {
    return (-a) * ((-b) * (-c));
  }

  if (typeof gczeal == 'function')
  {
    expect = f(1.5, 1.25, 1.125);
    gczeal(2);
    actual = f(1.5, 1.25, 1.125);
  }
  else
  {
    expect = actual = 'Test requires gczeal, skipped.';
  }

  if (typeof gczeal == 'function')
  {
    gczeal(0);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
