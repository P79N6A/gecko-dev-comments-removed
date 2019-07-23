




































var gTestfile = 'regress-355622.js';

var BUGNUMBER = 355622;
var summary = 'Do not assert: overwriting';
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
    (function() { export arguments })();
  }
  catch(ex)
  {
    print(ex + '');
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
