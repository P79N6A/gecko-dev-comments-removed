




































var bug = 350650;
var summary = 'js reports "uncaught exception';
var actual = 'Error';
var expect = 'Error';

expectExitCode(3);


test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  function exc() { this.toString = function() { return "EXC"; } }
  throw new exc();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
