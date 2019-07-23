




































var bug = 353214;
var summary = 'bug 353214';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f = function ([x]) { let x; }
  expect = 'function ([x]) { let x; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
