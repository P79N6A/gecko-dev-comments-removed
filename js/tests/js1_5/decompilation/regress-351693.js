




































var bug = 351626;
var summary = 'decompilation of ternary with parenthesized constant condition';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { (0) ? x : y }
  actual = f + '';
  expect = 'function () {\n    y;\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
