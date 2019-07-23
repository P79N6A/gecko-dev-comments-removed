




































var gTestfile = 'regress-351793.js';

var BUGNUMBER = 351793;
var summary = 'decompilation of double parenthesized object literal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { (({a:b, c:3})); }
  actual = f + '';
  expect = 'function () {\n    ({a:b, c:3});\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
