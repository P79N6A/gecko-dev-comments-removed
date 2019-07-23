




































var gTestfile = 'regress-349489.js';

var BUGNUMBER = 349489;
var summary = 'Incorrect decompilation of labeled useless statements';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function () {\nL:\n    3;\n}';
  var f = function() { L: 3; };
  actual = f.toString();
  print(f.toString());
  compareSource(expect, actual, summary);

  expect = 'function () {\nL:\n    3;\n    alert(5);\n}';
  f = function() { L: 3; alert(5); }
  actual = f.toString();
  print(f.toString());
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
