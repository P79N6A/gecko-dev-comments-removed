




































var gTestfile = 'regress-464334.js';

var BUGNUMBER = 464334;
var summary = 'Do not assert: (size_t) (fp->regs->sp - fp->slots) <= fp->script->nslots';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function g()
  {
    gc();
  }

  var a = [];
  for (var i = 0; i != 20; ++i)
    a.push(i);
  g.apply(this, a);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
