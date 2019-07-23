




































var gTestfile = 'regress-476655.js';

var BUGNUMBER = 476655;
var summary = 'TM: Do not assert: count <= (size_t) (fp->regs->sp - StackBase(fp) - depth)';
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
      "(function (){ for (var y in this) {} })();" +
      "[''.watch(\"\", function(){}) for each (x in ['', '', eval, '', '']) if " +
      "(x)].map(Function)"
      );
  }
  catch(ex)
  {
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
