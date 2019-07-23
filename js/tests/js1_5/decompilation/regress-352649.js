




































var gTestfile = 'regress-352649.js';

var BUGNUMBER = 352649;
var summary = 'decompilation of RegExp literal after |if| block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { if (x) { } (/a/gi.z); }
  expect = 'function() { if (x) { } /a/gi.z; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
