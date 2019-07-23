




































var gTestfile = 'regress-367501-01.js';

var BUGNUMBER = 367501;
var summary = 'getter/setter issues';
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
    expect = 'undefined';
    var a = { set x() {} };
    actual = a.x + '';
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
