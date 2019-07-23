




































var gTestfile = 'regress-456494.js';

var BUGNUMBER = 456494;
var summary = 'Do not crash with apply and argc > nargs';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function k(s)
  {
  }
  function f()
  {
    for (i = 0; i < 10; i++)
    {
      k.apply(this, arguments);
    }
  }
  f(1);

  jit(false);

  if (typeof this.tracemonkey != 'undefined')
  {
    for (var p in this.tracemonkey)
    {
      print(p + ':' + this.tracemonkey[p]);
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
