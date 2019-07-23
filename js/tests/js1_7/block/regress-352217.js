




































var bug = 352217;
var summary = 'Need space between |else|, |let|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;
  f = function() { if(g) h; else let x; }
  expect = 'function() { if(g) { h; } else let x; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
