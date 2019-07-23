




































var bug = 352272;
var summary = 'decompilation of |let| in arg to lvalue returning function';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;
  f = function() { f(let (y = 3) 4)++; }
  expect = 'function() { f((let (y = 3) 4))++; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
