




































var gTestfile = 'regress-351102-05.js';

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
    try { d.d.d } catch([] if gc()) { }
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary + ': 5');

  exitFunc ('test');
}
