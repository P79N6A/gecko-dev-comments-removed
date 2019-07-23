




































var gTestfile = 'regress-353146.js';

var BUGNUMBER = 353146;
var summary = 'Decompilation of new expressions revisited';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function () { return new (p(2))[1]; };
  expect = 'function() { return new (p(2)[1]); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function () { return new p(2)[1]; };
  expect = 'function () { return (new p(2))[1]; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
