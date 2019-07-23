




































var gTestfile = 'regress-355025.js';

var BUGNUMBER = 355025;
var summary = 'Test regression from bug 354750 - Iterable()';
var actual = 'No Error';
var expect = 'No Error';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  Iterator([]);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
