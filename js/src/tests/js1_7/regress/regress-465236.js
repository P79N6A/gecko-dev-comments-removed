




































var gTestfile = 'regress-465236.js';

var BUGNUMBER = 465236;
var summary = 'TM: Do not assert: we should have converted to numbers already';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);
  for (let j = 0; j < 2; ++j) null <= null;
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
