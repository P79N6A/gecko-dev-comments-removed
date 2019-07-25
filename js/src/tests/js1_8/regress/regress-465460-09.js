





































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

  for (let i in (function() { for (var j = 0; j < 3; ++j) yield; })()) { }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
