




































var bug = 352441;
var summary = 'Decompilation of case(yield)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { switch (x) { case (yield): } };
  expect = 'function() { switch (x) { case yield: default: ; } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
