




































var gTestfile = 'regress-345961.js';

var BUGNUMBER = 345961;
var summary = 'Array.prototype.shift should preserve holes';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = false;

  var array = new Array(2);
  array.shift();
  actual = array.hasOwnProperty(0);
  reportCompare(expect, actual, summary);

  array=Array(1);
  array.shift(1);
  actual = array.hasOwnProperty(1);
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
