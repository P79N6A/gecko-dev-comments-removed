




































var gTestfile = '11.4.1-002.js';

var BUGNUMBER = 423300;
var summary = '11.4.1 - The delete Operator - delete f()';
var actual = '';
var expect = '';



test();


function f() {}

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = true;

  try
  {
    actual = delete f();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
