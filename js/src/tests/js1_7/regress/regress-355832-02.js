




































var gTestfile = 'regress-355832-02.js';

var BUGNUMBER = 355832;
var summary = 'execution of let binding nothing';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = actual = 'No Crash';

  let ([] = []) { print(3) } print(4)

			       reportCompare(expect, actual, summary);

  exitFunc ('test');
}
