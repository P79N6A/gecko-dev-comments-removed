




































var gTestfile = 'regress-367501-04.js';

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
    for (var i = 0; i < 0x10050c - 3; ++i) a[i] = 1;
    a.x;
    typeof a.x;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
