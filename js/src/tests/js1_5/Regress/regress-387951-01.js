




































var gTestfile = 'regress-387951-01.js';

var BUGNUMBER = 387951;
var summary = 'Do not assert: cg->stackDepth >= 0';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (delete (0 ? 3 : {}));

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
