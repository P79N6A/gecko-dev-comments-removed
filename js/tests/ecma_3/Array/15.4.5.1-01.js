




































var gTestfile = '15.4.5.1-01.js';

var BUGNUMBER = "(none)";
var summary = '15.4.5.1 - array.length coverage';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var a = [];
 
  expect = 'RangeError: invalid array length';
  actual = '';
  try
  {
    a.length = -1;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  actual = '';
  try
  {
    a.length = 12345678901234567890;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  actual = '';
  try
  {
    a.length = 'a';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
