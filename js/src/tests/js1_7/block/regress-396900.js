




































var gTestfile = 'regress-396900.js';

var BUGNUMBER = 396900;
var summary = 'Destructuring bind in a let';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '3, 4';
  let ([x, y] = [3, 4]) { actual = x + ', ' + y}
  reportCompare(expect, actual, summary);

  expect = 'undefined, undefined';
  actual = typeof x + ', ' + typeof y;
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
