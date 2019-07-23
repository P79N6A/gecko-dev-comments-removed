




































var bug = 352453;
var summary = 'Decompilation of function() { (eval)(x)-- }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function() { (eval)(x)-- };
  expect = 'function() { eval(x)--; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
