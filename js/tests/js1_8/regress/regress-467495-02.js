




































var gTestfile = 'regress-467495-02.js';

var BUGNUMBER = 467495;
var summary = 'Do not crash @ js_Interpret';
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
    eval("(function(){const y = 1; function x() '' let functional, x})();");
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
