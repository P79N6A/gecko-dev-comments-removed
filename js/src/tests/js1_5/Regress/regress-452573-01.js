





var BUGNUMBER = 452573;
var summary = 'Do not assert with JIT: boxed.isUndefined() || JSVAL_IS_BOOLEAN(boxed)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  for(var j=0;j<5;++j) typeof void /x/;

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
