





var BUGNUMBER = 457065;
var summary = 'Do not assert: !fp->callee || fp->thisp == fp->argv[-1].toObjectOrNull()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function(){ eval('this'); (function(){ for(let y in [0,1,2]) 6;})(); })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
