




































var bug = 349818;
var summary = 'let expression should not assert';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  let (x=3) x;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
