




































var gTestfile = 'regress-479551.js';

var BUGNUMBER = 479551;
var summary = 'Do not assert: (cx)->requestDepth || (cx)->thread == (cx)->runtime->gcThread';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof shapeOf != 'function')
  {
    print(expect = actual = 'Test skipped: requires shell');
  }
  else
  {
    jit(true);

    var o = {a:3, b:2};
    shapeOf(o);
    var p = {};
    p.a =3;
    p.b=2;
    shapeOf(p);

    jit(false);

  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
