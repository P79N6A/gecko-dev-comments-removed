




































var bug = 353120;
var summary = 'decompilation of (new x)[y]++';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f;
  f = function() { return (new x)[y]++ }
  expect = 'function() { return (new x)[y]++; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
