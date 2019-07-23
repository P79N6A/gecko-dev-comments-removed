




































var gTestfile = 'regress-260106.js';

var BUGNUMBER = 260106;
var summary = 'Elisions in array literals should not create properties';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var g = (function f(a,b,c,d)[(a,b),(c,d)]);

  expect = 'function f(a,b,c,d)[(a,b),(c,d)]';
  actual = g + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
