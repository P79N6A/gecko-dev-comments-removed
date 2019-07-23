




































var bug = 350810;
var summary = 'decompilation for "let" in lvalue part of for..in';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f = function () { for (let i = 3 in {}); }
  actual = f + '';
  expect = 'function () {\n    for (let i in {}) {\n    }\n}';
  compareSource(expect, actual, summary);

  var f = function () { for (let i = (y = 4) in {}); }
  actual = f + '';
  expect = 'function () {\n    y = 4;\n    for (let i in {}) {\n    }\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
