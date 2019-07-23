




































var gTestfile = 'regress-452498-114-a.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);



  if (typeof timeout == 'function')
  {
    timeout(3);
    try
    {
      eval('while(x|=#3={}) with({}) const x;');
    }
    catch(ex)
    {
    }
    reportCompare(expect, actual, '');
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
