




































var gTestfile = 'regress-349650.js';

var BUGNUMBER = 349650;
var summary = 'Number getting parens replaces last character of identifier in decompilation of array comprehension';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function() { [5[7] for (y in window)]; }
  expect = 'function () {\n    [(5)[7] for (y in window)];\n}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
