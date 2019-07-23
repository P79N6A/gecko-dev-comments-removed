




































var bug = 352606;
var summary = 'Do not crash involving post decrement';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  y = ({toString: gc}); new Function("y--;")()

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
