




































var gTestfile = 'regress-426711.js';

var BUGNUMBER = 426711;
var summary = 'Setting window.__count__ causes a crash';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof window != 'undefined' && '__count__' in window)
  {
    window.__count__ = 0;
  }
  else
  {
    expect = actual = 'Test skipped. Requires window.__count__';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
