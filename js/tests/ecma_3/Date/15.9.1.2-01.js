




































var gTestfile = '15.9.1.2-01.js';

var BUGNUMBER = 264727;
var summary = '15.9.1.2 - TimeWithinDay(TIME_1900) == 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 0;
  actual = TimeWithinDay(TIME_1900);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
