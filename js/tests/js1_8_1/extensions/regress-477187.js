




































var gTestfile = 'regress-477187.js';

var BUGNUMBER = 477187;
var summary = 'timeout script';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof window != 'undefined' || typeof timeout != 'function')
  {
    print(expect = actual = 'Test skipped due to lack of timeout function');
    reportCompare(expect, actual, summary);
  }
  else
  {
    timeout(3); 
    
    
    
    reportCompare(expect, actual, summary);

    while(1); 
  }


  exitFunc ('test');
}
