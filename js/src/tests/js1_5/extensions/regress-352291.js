




































var gTestfile = 'regress-352291.js';

var BUGNUMBER = 352291;
var summary = 'disassembly of regular expression';
var actual = '';
var expect = 'TypeError: /g/g is not a function';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof dis != 'function')
  {
    actual = expect = 'disassembly not supported, test skipped.';
  }
  else
  {
    try
    {
      dis(/g/g)
    }
    catch(ex)
    {
      actual = ex + '';
    }
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
