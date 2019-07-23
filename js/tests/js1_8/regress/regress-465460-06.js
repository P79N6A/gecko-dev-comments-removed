




































var gTestfile = 'regress-465460-06.js';

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

  for each (let x in [1, {}, 1, null, 1, {}, 1, null, 1]) { }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
