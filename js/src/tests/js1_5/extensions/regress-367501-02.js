




































var gTestfile = 'regress-367501-02.js';

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
    expect = 'undefined';
    var a = { set x() {} };
    for (var i = 0; i < 92169 - 3; ++i) a[i] = 1;
    actual = a.x + '';
    actual = a.x + '';
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
