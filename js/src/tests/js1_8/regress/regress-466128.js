




































var gTestfile = 'regress-466128.js';

var BUGNUMBER = 466128;
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
  for (let a = 0; a < 3; ++a) { 
    for each (let b in [1, 2, "three", 4, 5, 6, 7, 8]) {
      }
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
