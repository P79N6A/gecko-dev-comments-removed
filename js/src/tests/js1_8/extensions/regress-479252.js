




































var gTestfile = 'regress-479252.js';

var BUGNUMBER = 479252;
var summary = 'Avoid watchdog ticks when idle in shell';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof sleep != 'function' || typeof scatter != 'function' ||
      typeof timeout != 'function')
  {
    print(expect = actual = 'Test skipped: requires mulithreads and timeout.');
  }
  else
  {
    expectExitCode(6);

    function f() { sleep(100); }
    timeout(1.0);
    scatter([f,f,f,f,f]);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
