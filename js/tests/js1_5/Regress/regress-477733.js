




































var gTestfile = 'regress-477733.js';

var BUGNUMBER = 477733;
var summary = 'TM: Do not assert: !(fp->flags & JSFRAME_POP_BLOCKS)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  function g() {
    [];
  }

  try {
    d.d.d;
  } catch(e) {
    void (function(){});
  }

  for (var o in [1, 2, 3]) {
    g();
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
