





































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
 
  actual = "no exception";
  try
  {
    expect = 'TypeError';
    var obj = {a: 5}; obj.__iterator__ = /x/g; for(x in y = let (z) obj) { }
  }
  catch(ex)
  {
    actual = ex instanceof TypeError ? 'TypeError' : "" + ex;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
