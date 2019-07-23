




































var bug = 379483;
var summary = 'Assertion: top < ss->printer->script->depth';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  (function () { try { } catch([e]) { [1]; } });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
