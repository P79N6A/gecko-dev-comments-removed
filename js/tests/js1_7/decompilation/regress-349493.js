




































var bug = 349493;
var summary = 'Decompilation of let expression in ternary';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 'function (x) {\n    return x ? 0 : (let (a = 3) a);\n}';

  var f = function (x) { return x ? 0 : let (a = 3) a; }
  actual = f.toString();
  print(f.toString());
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
