




































var gTestfile = 'regress-443074.js';

var BUGNUMBER = 443074;
var summary = 'Decompilation of genexp in for loop condition';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { for(; x || (1 for each (y in [])); ) { } };
  expect = 'function () { for(; x || true; ) { } }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
