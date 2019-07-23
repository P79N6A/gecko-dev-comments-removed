




































var bug = 354910;
var summary = 'decompilation of (delete r(s)).t = a;';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f = function () { (delete r(s)).t = a; }

  expect = 'function () { (delete r(s)).t = a; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
