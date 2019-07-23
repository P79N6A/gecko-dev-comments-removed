




































var gTestfile = 'regress-352202.js';

var BUGNUMBER = 352202;
var summary = 'decompilation of for ((~x)["y"] in z)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { for ((~x)["y"] in z) { } }
  expect = 'function() { for ((~x).y in z) { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
