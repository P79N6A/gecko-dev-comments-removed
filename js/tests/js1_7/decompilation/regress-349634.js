




































var bug = 349634;
var summary = 'decompilation of {} and let';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f;

  f = function () { let a = 3; { let a = 4; } }
  actual = f + '';
  expect = 'function () {\n    let a = 3;\n    {\n        let a = 4;\n    }\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
