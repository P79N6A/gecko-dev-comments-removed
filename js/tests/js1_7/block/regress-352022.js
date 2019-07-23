




































var bug = 352022;
var summary = 'decompilation of let, delete and parens';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { g(h) = (delete let (y) 3); }
  actual = f + '';
  expect = 'function () {\n    g(h) = ((let (y) 3), true);\n}';
  compareSource(expect, actual, summary);

  f = function () {    g(h) = ((let (y) 3), true);}
  actual = f + '';
  expect = 'function () {\n    g(h) = ((let (y) 3), true);\n}';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
