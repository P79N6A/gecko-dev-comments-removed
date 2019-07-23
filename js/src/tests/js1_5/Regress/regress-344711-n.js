




































var gTestfile = 'regress-344711-n.js';

var BUGNUMBER = 344711;
var summary = 'Do not crash compiling when peeking over a newline';
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
    
    
    var a1 = {abc2 : 1, abc3 : 3};
    var j = eval('a1\\\n.abc2;');
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
