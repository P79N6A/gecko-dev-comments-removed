




































var gTestfile = 'regress-349633.js';

var BUGNUMBER = 349633;
var summary = 'Decompilation of increment/decrement on let bound variable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function () { let (x = 3) { x-- } }
  expect = 'function () {\n    let (x = 3) {\n        x--;\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function () { let (x = 3) { x--; x--; } }
  expect = 'function () {\n    let (x = 3) {\n        x--;\n        x--;\n    }\n}'
    actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
