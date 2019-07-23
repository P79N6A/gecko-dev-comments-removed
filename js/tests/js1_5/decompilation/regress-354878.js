




































var bug = 354878;
var summary = 'decompilation of |if(0) { LABEL: const x;}|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function () { if(0) { L: const x; } return x; }
  expect = 'function () { if(0) { L: const x; } return x; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
