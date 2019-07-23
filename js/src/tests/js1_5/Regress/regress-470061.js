




































var gTestfile = 'regress-470061.js';

var BUGNUMBER = 470061;
var summary = 'TM: Do not assert: cx->fp->regs->pc == f->ip && f->root == f';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);
 
  function x (w, z) {
    var h = 0;
    var q = 0;
    while (q < 300) {
      while (w) {
      }
      ++q;
      if (q % 4 == 1) {
        h = Math.ceil(z);
      }
    }
  }

  x(false, 40);

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
