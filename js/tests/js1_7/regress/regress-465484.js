




































var gTestfile = 'regress-465484.js';

var BUGNUMBER = 465484;
var summary = 'TM: Do not assert: _allocator.active[FST0] && _fpuStkDepth == -1 || ' +
  '!_allocator.active[FST0] && _fpuStkDepth == 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);
  for each (let a in [2, 2, 2]) { a %= a; a %= a; }
  jit(false);
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
