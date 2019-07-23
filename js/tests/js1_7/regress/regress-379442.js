




































var bug = 379442;
var summary = 'Regression from bug 368224';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  reportCompare(expect, actual, summary);

  function () { ({ y: [] }) = {} }

  exitFunc ('test');
}
