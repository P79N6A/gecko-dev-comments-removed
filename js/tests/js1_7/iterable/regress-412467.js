




































var gTestfile = 'regress-412467.js';

var BUGNUMBER = 412467;
var summary = 'Iterator values in array comprehension';
var actual = '';
var expect = 'typeof(iterand) == undefined, ';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function g() { yield 1; yield 2; }

  var a = [iterand for (iterand in g())];

  expect = true;
  actual = typeof iterand == 'undefined';
  reportCompare(expect, actual, summary + ': typeof iterand == \'undefined\'');

  expect = true;
  actual = a.length == 2 && a.toString() == '1,2';
  reportCompare(expect, actual, summary + ': a.length == 2 && a.toString() == \'1,2\'');

  exitFunc ('test');
}
