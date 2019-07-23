




































var gTestfile = 'regress-428706.js';

var BUGNUMBER = 428706;
var summary = 'Do not assert: regs.sp < vp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  [1 for ([,,] in [])];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
