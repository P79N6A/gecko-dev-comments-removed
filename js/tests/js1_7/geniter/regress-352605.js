




































var gTestfile = 'regress-352605.js';

var BUGNUMBER = 352605;
var summary = 'Do not assert with |yield|, nested xml-filtering predicate';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  { 
    (function() { <y/>.(<x/>.(false), (yield 3)) })().next();
  }
  catch(ex)
  {
    print(ex + '');
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
