




































var gTestfile = 'regress-472450-03.js';

var BUGNUMBER = 472450;
var summary = 'TM: Do not assert: StackBase(fp) + blockDepth == regs.sp';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  ({__proto__: #1=[#1#]})
    for (var y = 0; y < 3; ++y) { for each (let z in ['', function(){}]) { let x =
                                               1, c = []; } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
