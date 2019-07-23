




































var gTestfile = 'regress-352885-01.js';

var BUGNUMBER = 352885;
var summary = 'Do not crash iterating over gen.__proto__';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (j in ((function() { yield 3; })().__proto__))
    print(j);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
