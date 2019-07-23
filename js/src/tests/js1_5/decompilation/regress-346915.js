




































var gTestfile = 'regress-346915.js';

var BUGNUMBER = 346915;
var summary = 'Optimize decompilation of delete expressions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { delete 3; };
  expect = 'function () {\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
