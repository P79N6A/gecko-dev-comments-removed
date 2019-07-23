




































var gTestfile = 'regress-454744.js';

var BUGNUMBER = 454744;
var summary = 'Do not assert with JIT: PCVAL_IS_SPROP(entry->vword)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  try
  {
    this.__defineGetter__('x', function() 2); for (var j=0;j<4;++j) { x=1; }
  }
  catch(ex)
  {
    print(ex + '');
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
