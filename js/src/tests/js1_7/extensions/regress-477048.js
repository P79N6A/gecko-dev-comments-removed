




































var gTestfile = 'regress-477048.js';

var BUGNUMBER = 477048;
var summary = 'Do not assert: cg->stackDepth == loopDepth';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    for each (this.__proto__ in x) {}
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
