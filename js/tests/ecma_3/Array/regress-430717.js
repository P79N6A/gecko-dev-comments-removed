




































var gTestfile = 'regress-430717.js';

var BUGNUMBER = 430717;
var summary = 'Dense Arrays should inherit deleted elements from Array.prototype';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  Array.prototype[2] = "two";
  var a = [0,1,2,3];
  delete a[2];

  expect = 'two';
  actual = a[2];
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
