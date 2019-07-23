




































var bug = 355635;
var summary = 'execution of let binding nothing';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = actual = 'No Crash';

  let ([] = []) { }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
