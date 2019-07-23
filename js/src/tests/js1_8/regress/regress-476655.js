




































var gTestfile = 'regress-476655.js';

var BUGNUMBER = 476655;
var summary = 'Do not assert: depth <= (size_t) (fp->regs->sp - StackBase(fp))';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  try
  {
    eval(
      "for(let y in ['', '']) try {for(let y in ['', '']) ( /x/g ); } finally {" +
      "with({}){} } this.zzz.zzz"

      );
  }
  catch(ex)
  {
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
