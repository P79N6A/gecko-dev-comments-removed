




































var gTestfile = 'regress-355820.js';

var BUGNUMBER = 355820;
var summary = 'Remove non-standard Script object';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  print('This test will fail in gecko prior to 1.9');
 
  expect = 'undefined';
  actual = typeof Script;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
