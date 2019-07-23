




































var gTestfile = 'regress-352312.js';

var BUGNUMBER = 352312;
var summary = 'decompilation of |new| with unary expression';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { new (-2) }
  expect = 'function() { new (-2); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  var f;
  f = function (){(new x)(y)}
  expect = 'function (){(new x)(y);}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
