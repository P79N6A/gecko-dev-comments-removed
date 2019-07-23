




































var gTestfile = 'regress-353454.js';

var BUGNUMBER = 353454;
var summary = 'Do not assert with regexp iterator';
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
    expect = 'TypeError: y.__iterator__ returned a primitive value';
    var obj = {a: 5}; obj.__iterator__ = /x/g; for(x in y = let (z) obj) { }
    expect = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
