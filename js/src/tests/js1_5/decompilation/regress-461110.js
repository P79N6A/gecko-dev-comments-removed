




































var gTestfile = 'regress-461110.js';

var BUGNUMBER = 461110;
var summary = 'Decompilation of a += b = 3';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = (function() { a += b = 3 });

  expect = 'function() { a += b = 3; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
