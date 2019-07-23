




































var gTestfile = 'regress-420919.js';

var BUGNUMBER = 420919;
var summary = 'this.u.v = 1 should report this.u is undefined';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  
  expect = /TypeError: this.u is undefined|TypeError: this.u has no properties/;

  try
  {
    this.u.v = 1;
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
