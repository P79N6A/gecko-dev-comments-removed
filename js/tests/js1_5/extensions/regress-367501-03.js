




































var gTestfile = 'regress-367501-03.js';

var BUGNUMBER = 367501;
var summary = 'getter/setter crashes';
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
    expect = actual = 'No Crash';
    var a = { set x() {} };
    for (var i = 0; i < 0x4bf20 - 3; ++i) a[i] = 1;
    a.x;
    a.x.x;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
