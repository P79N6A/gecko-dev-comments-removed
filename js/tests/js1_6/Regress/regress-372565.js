




































var gTestfile = 'regress-372565.js';

var BUGNUMBER = 372565;
var summary = 'Do not assert: top < ss->printer->script->depth" decompiling a function where a const identifier is used as a for-loop variable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  (function() { for each(x in y) { } const x; });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
