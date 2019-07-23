




































var gTestfile = 'regress-462282.js';

var BUGNUMBER = 462282;
var summary = 'Do not assert: !ti->stackTypeMap.matches(ti_other->stackTypeMap)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for each (let i in [0, 0, 0, "", 0, 0, "", 0, 0, "", 0]) { }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
