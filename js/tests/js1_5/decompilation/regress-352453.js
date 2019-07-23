




































var gTestfile = 'regress-352453.js';

var BUGNUMBER = 352453;
var summary = 'Decompilation of function() { (eval)(x)-- }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { (eval)(x)-- };
  expect = 'function() { eval(x)--; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
