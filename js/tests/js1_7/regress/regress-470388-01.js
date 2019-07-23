




































var gTestfile = 'regress-470388-01.js';

var BUGNUMBER = 470388;
var summary = 'TM: Do not assert: !(fp->flags & JSFRAME_POP_BLOCKS)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for each (let x in [function(){}, new Boolean(false), function(){}]) {}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
