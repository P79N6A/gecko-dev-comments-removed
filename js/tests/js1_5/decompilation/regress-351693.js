




































var gTestfile = 'regress-351693.js';

var BUGNUMBER = 351693;
var summary = 'decompilation of ternary with parenthesized constant condition';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { (0) ? x : y }
  actual = f + '';
  expect = 'function () {\n    y;\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
