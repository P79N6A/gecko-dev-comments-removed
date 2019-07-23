




































var gTestfile = 'regress-350670.js';

var BUGNUMBER = 350670;
var summary = 'decompilation of (for(z() in x)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function() { for(z() in x) { } }
  expect = 'function () {\n    for (z() in x) {\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { for(z(12345678) in x) { } }
  expect = 'function () {\n    for (z(12345678) in x) {\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
