




































var bug = 352025;
var summary = 'decompilation of nested yields';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;
  f = function() { yield (yield a); }
  expect = 'function() { yield (yield a); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
