





var BUGNUMBER = 353214;
var summary = 'bug 353214';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function ([x]) { let y; }
  expect = 'function ([x]) { let y; }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
