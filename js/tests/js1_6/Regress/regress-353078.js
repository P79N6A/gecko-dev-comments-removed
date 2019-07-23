




































var gTestfile = 'regress-353078.js';

var BUGNUMBER = 353078;
var summary = 'Do not assert with bogus toString, map, split';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'TypeError: can\'t convert global to string';
  try
  {
    this.toString = function() { return {}; }; p = [11].map('foo'.split);
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
