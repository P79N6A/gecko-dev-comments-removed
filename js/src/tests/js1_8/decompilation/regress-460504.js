




































var gTestfile = 'regress-460504.js';

var BUGNUMBER = 460504;
var summary = 'Decompilation of genexp in for-loop condition';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function () { for(; x, (1 for each (y in [])); ) { } }';
  f = (function () { for(; x, (1 for each (y in [])); ) { } });
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
