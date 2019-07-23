






var gTestfile = 'regress-479381.js';

var BUGNUMBER = 479381;
var summary = 'Do not crash @ js_FinalizeStringRT with multi-threads.';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof gczeal != 'function' || typeof scatter != 'function')
  {
    print(expect = actual = 'Test skipped: requires mulithreads');
  }
  else
  {
    expect = actual = 'No Crash';

    gczeal(2);

    function f() {
      var s;
      for (var i = 0; i < 9999; i++)
        s = 'a' + String(i)[3] + 'b';
      return s;
    }

    print(scatter([f, f, f, f]));

    gczeal(0);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
