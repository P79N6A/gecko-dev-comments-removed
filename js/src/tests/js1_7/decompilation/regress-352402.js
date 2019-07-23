




































var gTestfile = 'regress-352402.js';

var BUGNUMBER = 352402;
var summary = 'decompilation of labelled block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { L: { let x; } }
  actual = f + '';
  expect = 'function() { L: { let x; } } ';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
