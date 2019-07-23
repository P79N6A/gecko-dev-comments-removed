




































var gTestfile = 'regress-375711.js';

var BUGNUMBER = 375711;
var summary = 'Do not assert with /[Q-b]/i.exec("")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    expect = 'SyntaxError: invalid range in character class';
    eval('/[Q-b]/i.exec("")');
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
