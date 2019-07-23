




































var gTestfile = 'regress-350387.js';

var BUGNUMBER = 350387;
var summary = 'Var declaration and let with same name';
var actual = '';
var expect = '';

expect = undefined + '';
actual = '';
let (x = 2)
{
  var x;
}
actual = x + '';
reportCompare(expect, actual, summary);


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = undefined + '';
  actual = '';
  (function () { let (x = 2) { var x; } actual = x + ''; })(); 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
