





































var gTestfile = 'regress-451906.js';

var BUGNUMBER = 451906;
var summary = 'Index array by numeric string';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 1; 
  var s=[1,2,3];
  actual = s['0'];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
