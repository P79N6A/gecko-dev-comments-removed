





































var bug = 346642;
var summary = 'decompilation of destructuring assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function f() { [z] = 3 }
  expect = 'function f() { [z] = 3; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = actual = 'No Crash';
  print(uneval(f));
  reportCompare(expect, actual, 'print(uneval(f))');
  exitFunc ('test');
}
