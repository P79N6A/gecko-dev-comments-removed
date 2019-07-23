




































var gTestfile = 'regress-351625.js';

var BUGNUMBER = 351625;
var summary = 'decompilation of object literal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;
  f = function() { ({a:b, c:3}); }
  expect = 'function () {\n    ({a:b, c:3});\n}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
