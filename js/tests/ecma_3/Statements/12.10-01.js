




































var gTestfile = '12.10-01.js';

var BUGNUMBER = 462734;
var summary = 'evaluating lhs "Reference" *before* evaluating rhs';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var x = 1;
  var o = {};
  with (o)
    x = o.x = 2;
  print(x);

  expect = 4;
  actual = x + o.x;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
