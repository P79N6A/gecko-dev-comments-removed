




































var gTestfile = 'regress-436741.js';

var BUGNUMBER = 436741;
var summary = 'Do not assert: OBJ_IS_NATIVE(obj)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof window == 'undefined')
  {
    print('This test is only meaningful in the browser.');
  }
  else
  {
    window.__proto__.__proto__ = [{}];
    for (var j in window);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
