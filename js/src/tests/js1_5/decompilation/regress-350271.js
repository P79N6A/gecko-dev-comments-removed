




































var gTestfile = 'regress-350271.js';

var BUGNUMBER = 350271;
var summary = 'decompilation if (0x10.(a))';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { if(0x10.(a)) { h(); } }
  expect = 'function () {\n    if ((16).(a)) {\n        h();\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
