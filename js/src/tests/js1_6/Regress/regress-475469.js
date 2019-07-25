





































var BUGNUMBER = 475469;
var summary = 'TM: Do not crash @ FramePCOffset';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);





  jit(true);
  [1,2,3].map(function(v, i, t) { return /a/gi.exec(v); });
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

