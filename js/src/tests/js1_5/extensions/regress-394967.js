




































var gTestfile = 'regress-394967.js';

var BUGNUMBER = 394967;
var summary = 'Do not assert: !JSVAL_IS_PRIMITIVE(vp[1])';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof evalcx == 'undefined')
  {
    print('Skipping. This test requires evalcx.');
  }
  else
  {
    var sandbox = evalcx(""); 
    try
    {
      evalcx("(1)()", sandbox);
    }
    catch(ex)
    {
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
