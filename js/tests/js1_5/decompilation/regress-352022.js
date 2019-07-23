




































var gTestfile = 'regress-352022.js';

var BUGNUMBER = 352022;
var summary = 'decompilation old, bad bug dropping parenthesis';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function (){a[b] = (c, d)}
  actual = f + '';
  expect = 'function () {\n    a[b] = (c, d);\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
