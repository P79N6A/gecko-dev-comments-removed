




































var gTestfile = 'regress-457065-02.js';

var BUGNUMBER = 457065;
var summary = 'Do not assert: !fp->callee || fp->thisp == JSVAL_TO_OBJECT(fp->argv[-1])';
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
