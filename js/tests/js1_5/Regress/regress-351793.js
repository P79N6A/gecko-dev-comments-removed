




































var bug = 351793;
var summary = 'decompilation of double parenthesized object literal';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { (({a:b, c:3})); } 
  actual = f + '';
  expect = 'function () {\n    ({a:b, c:3});\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
