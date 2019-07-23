




































var gTestfile = 'regress-470300-01.js';

var BUGNUMBER = 470300;
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

  for (let a = 0; a < 3; ++a) { let b = '' + []; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
