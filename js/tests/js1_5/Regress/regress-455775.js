




































var gTestfile = 'regress-455775.js';

var BUGNUMBER = 455775;
var summary = 'Do not assert: cx->fp->flags & JSFRAME_EVAL';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    (function() { var c; eval("new (c ? 1 : {});"); })();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
