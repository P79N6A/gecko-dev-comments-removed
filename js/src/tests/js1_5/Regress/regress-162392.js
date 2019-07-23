




































var gTestfile = 'regress-162392.js';




var BUGNUMBER = 162392;
var summary = 'eval("arguments").length == 0 when no arguments specified';
var actual = noargslength();
var expect = 0;

function noargslength()
{
  enterFunc('noargslength');
  return eval('arguments').length;
  exitFunc('noargslength');
}


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
