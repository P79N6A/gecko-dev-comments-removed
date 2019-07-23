




































var bug = 349499;
var summary = 'Decompilation of new let';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 'function (x) {\n    new (let (x = 3) x);\n}';

  var f = function(x) { new let (x = 3) x };
  actual = f.toString();
  print(f.toString());
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
