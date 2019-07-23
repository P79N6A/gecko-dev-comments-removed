




































var gTestfile = 'regress-452498-091.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);



  expect = 'function eval() {\n    eval("v");\n}';
  f = (function eval() { eval("v"); });
  actual = f + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
