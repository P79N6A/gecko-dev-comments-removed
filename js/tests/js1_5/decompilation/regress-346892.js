




































var gTestfile = 'regress-346892.js';

var BUGNUMBER = 346892;
var summary = 'decompilation of new Function("3")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function anonymous() {\n}';
  actual = (new Function("3")) + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
