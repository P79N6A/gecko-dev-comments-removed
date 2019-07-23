




































var gTestfile = 'regress-351795.js';

var BUGNUMBER = 351795;
var summary = 'Do not assert: top < ss->printer->script->depth';
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
    p={}; 
    (p.z = [1].some(function(y) { return y > 0; }) ? 4 : [6])(5);
  }
  catch(ex)
  {
    print(ex + '');
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
