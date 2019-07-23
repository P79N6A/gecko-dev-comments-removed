




































var bug = 355004;
var summary = 'decompilation of |[,] =x|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function () { [,] = x; }
  expect = 'function () { [,] = x; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
