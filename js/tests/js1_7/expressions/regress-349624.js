




































var bug = 349624;
var summary = 'let in initial-value expression for another let';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  let(y = let (x) 4) 3

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
