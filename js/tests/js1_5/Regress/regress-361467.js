




































var bug = 361467;
var summary = 'Do not crash with certain watchers';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = actual = 'No Crash';

  var x;
  this.watch('x', print); 
  x = 5;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
