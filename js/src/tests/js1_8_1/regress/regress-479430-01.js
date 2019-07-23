




































var gTestfile = 'regress-479430-01.js';

var BUGNUMBER = 479430;
var summary = 'Missing operation callback checks';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof timeout == 'function')
  {
    expectExitCode(6);

    timeout(0.01);

    function f(n)
    {
      if (n != 0) {
        f(n - 1);
        f(n - 1);
      }
    }

    f(100);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
