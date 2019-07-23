




































var gTestfile = 'regress-465460-12.js';

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

  for (var j = 0; j < 2; ++j) { if (null > "") { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
