






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
        try { f(n - 1); } finally { f(n - 1); }
      }
      name_that_does_not_exists;
    }

    f(100);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
