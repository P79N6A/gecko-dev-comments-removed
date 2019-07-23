




































var gTestfile = 'regress-472703.js';

var BUGNUMBER = 472703;
var summary = 'Do not assert: regs.sp[-1] == OBJECT_TO_JSVAL(fp->scopeChain)';
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
      'for (var z = 0; z < 2; ++z) { with({}) for(let y in [1, null]); let(x)' +
      '(function(){})(); }'
      );
  }
  catch(ex)
  {
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
