




































var gTestfile = 'regress-352269.js';

var BUGNUMBER = 352269;
var summary = 'decompilation of |yield(1,2)|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { yield (1,2) }
  expect = 'function() { yield (1,2); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
