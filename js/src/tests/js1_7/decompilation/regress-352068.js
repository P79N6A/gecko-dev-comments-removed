




































var gTestfile = 'regress-352068.js';

var BUGNUMBER = 352068;
var summary = 'decompilation of !(3)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { return !(3) }
  expect = 'function() { return false; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
