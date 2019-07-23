




































var gTestfile = 'regress-442333-01.js';

var BUGNUMBER = 442333;
var summary = 'Remove eval\'s optional second argument';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'ReferenceError: a is not defined';
  var o = {a : 1};

  try
  {
    eval('a', o);
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
