




































var gTestfile = 'regress-375642.js';

var BUGNUMBER = 375642;
var summary = 'RegExp /(?:a??)+?/.exec("")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  /(?:a??)+?/.exec("")

     reportCompare(expect, actual, summary);

  exitFunc ('test');
}
