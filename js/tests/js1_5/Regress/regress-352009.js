




































var bug = 352009;
var summary = 'Do not assert [1 for (y in [3])]';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  [1 for (y in [3])];
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
