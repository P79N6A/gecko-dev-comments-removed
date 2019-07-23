




































var gTestfile = 'regress-461233.js';

var BUGNUMBER = 461233;
var summary = 'Decompilation of function () [(1,2)]';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  f = (function() [(1, 2)]);

  expect = 'function() [(1, 2)]';
  actual = f + '';

  compareSource(expect, actual, expect);

  exitFunc ('test');
}
