




































var bug = 349596;
var summary = 'decompilation of labeled if(0)...';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f = function() { L: if (0) return 5 }

  expect = 'function () {\n    L: {\n    }\n}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
