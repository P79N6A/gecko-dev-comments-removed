




































var bug = 352268;
var summary = 'decompilation should not change scope of |let| in |else|...|if|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  f = function() { if(x) { } else if (y) let b=2; }
  expect = 'function() { if(x) { } else if (y) let b=2; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
