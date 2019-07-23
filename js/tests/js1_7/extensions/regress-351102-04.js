




































var gTestfile = 'regress-351102-04.js';

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
    try { @foo } catch([] if gc()) { }
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary + ': 4');

  exitFunc ('test');
}
