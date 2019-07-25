





var BUGNUMBER = 465460;
var summary = 'TM: valueOf in a loop: do not assert';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let i = 0; i < 2; ++i) { ({}) + 3; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
