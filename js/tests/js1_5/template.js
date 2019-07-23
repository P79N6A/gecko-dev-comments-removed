




































var bug = 99999;
var summary = '';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
