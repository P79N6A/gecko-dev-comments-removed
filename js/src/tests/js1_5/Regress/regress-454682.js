




































var gTestfile = 'regress-454682.js';

var BUGNUMBER = 454682;
var summary = 'Do not crash with JIT in MatchRegExp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  var a = new String("foo");
  for (i = 0; i < 300; i++) {
    a.match(/bar/);
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
