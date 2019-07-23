




































var bug = 352198;
var summary = 'decompilation of yield (yield)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { yield (yield); }
  expect = 'function() { yield (yield); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
