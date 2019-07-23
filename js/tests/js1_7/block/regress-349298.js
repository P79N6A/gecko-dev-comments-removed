




































var bug = 349283;
var summary = 'Do not bogo assert';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  function() { for(i=0;i<4;++i) let x = 4; }
  
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
