




































var gTestfile = 'regress-355834.js';

var BUGNUMBER = 355834;
var summary = 'new Function("yield")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '[object Generator]';
  var g = (new Function('yield'))(1);
  actual = g + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
