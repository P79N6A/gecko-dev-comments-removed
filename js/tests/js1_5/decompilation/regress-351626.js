




































var bug = 351626;
var summary = 'decompilation of if(lamda)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f;

  f = function () { if (function () {}) { g(); } }
  actual = f + '';
  expect = 'function () {\n    if (function () {}) {\n        g();\n    }\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
