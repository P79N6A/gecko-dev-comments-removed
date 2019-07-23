




































var gTestfile = 'regress-352732.js';

var BUGNUMBER = 352732;
var summary = 'Decompiling "if (x) L: let x;"';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function() { if (x) L: let x; });
  expect = 'function() { if (x) { L: let x; } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = (function() { if (x) L: let x; else y; });
  expect = 'function() { if (x) { L: let x; } else { y;} }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
