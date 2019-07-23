




































var gTestfile = 'regress-426827.js';

var BUGNUMBER = 426827;
var summary = 'Do not assert: !(js_CodeSpec[op2].format & JOF_DEL)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    eval('eval()=delete a;');
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
