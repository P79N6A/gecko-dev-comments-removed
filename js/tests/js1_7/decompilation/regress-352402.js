




































var bug = 352402;
var summary = 'decompilation of labelled block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function() { L: { let x; } } 
  actual = f + '';
  expect = 'function() { L: { let x; } } ';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
