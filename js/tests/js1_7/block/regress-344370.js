




































var gTestfile = 'regress-344370.js';

var BUGNUMBER = 344370;
var summary = 'let declaration in let statement shows assertion';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  let (a = 2) { let b = 3; [a, b]; }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
