




































var gTestfile = 'regress-352212.js';

var BUGNUMBER = 352212;
var summary = 'Do not crash with XML filtering predicate, |let|, string.replace';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /TypeError: /;

  try
  {
    'a'.replace(/a/g, function () { return let(y) (3).(<x/>) });
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
