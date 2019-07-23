




































var gTestfile = 'regress-457824.js';

var BUGNUMBER = 457824;
var summary = 'decompilation of new a(b).c';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function() { (new a(b)).c; }';
  var g = (function() { new a(b).c });

  actual = g + '';
  compareSource(expect, actual, summary);

  actual = g + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
