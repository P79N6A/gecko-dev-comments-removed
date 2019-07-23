




































var gTestfile = 'regress-452168.js';

var BUGNUMBER = 452168;
var summary = 'Do not crash with gczeal 2, JIT: @ avmplus::List or @ nanojit::LirBuffer::validate';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof gczeal == 'undefined')
  {
      expect = actual = 'Test requires gczeal, skipped.';
  }
  else
  {
    jit(true);
    gczeal(2);

    var a, b; gczeal(2); (function() { for (var p in this) { } })();

    gczeal(0);
    jit(false);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
