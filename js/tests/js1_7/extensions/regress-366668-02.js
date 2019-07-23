




































var bug = 366668;
var summary = 'decompilation of "let with with" ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { let (w) { with({x: w.something }) { } } };
  expect = 'function() { let (w) { with({x: w.something }) { } } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
