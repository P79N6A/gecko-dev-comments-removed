




































var gTestfile = 'regress-351102-06.js';

var BUGNUMBER = 351102;
var summary = 'try/catch-guard/finally GC issues';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  try
  {
    try { null.a } catch(e if (e = null, gc())) { }
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary + ': 6');

  exitFunc ('test');
}
