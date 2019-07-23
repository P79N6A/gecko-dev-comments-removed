




































var bug = 355025;
var summary = 'Test regression from bug 354750 - Iterable()';
var actual = 'No Error';
var expect = 'No Error';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  Iterator([]);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
