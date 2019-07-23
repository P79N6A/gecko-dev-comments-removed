




































var gTestfile = 'regress-454704.js';

var BUGNUMBER = 454704;
var summary = 'Do not crash with defineGetter and XPC wrapper';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof XPCSafeJSObjectWrapper != 'undefined' && typeof document != 'undefined')
  {
    gDelayTestDriverEnd = true;
    document.addEventListener('load', boom, true);
  }
  else
  {
    print(expect = actual = 'Test requires browser.');
    reportCompare(expect, actual, summary);
  }
  exitFunc ('test');
}

function boom()
{
  try
  {
    var a = [];
    g = [];
    g.__defineGetter__("f", g.toSource);
    a[0] = g;
    a[1] = XPCSafeJSObjectWrapper(a);
    print("" + a);
  }
  catch(ex)
  {
    print(ex + '');
  }
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
  reportCompare(expect, actual, summary);
}

