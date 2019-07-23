




































var gTestfile = 'regress-466905-05.js';

var BUGNUMBER = 466905;
var summary = 'Sandbox shapes';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof evalcx != 'function')
  {
    expect = actual = 'Test skipped: requires evalcx support';
  }
  else if (typeof shapeOf != 'function')
  {
    expect = actual = 'Test skipped: requires shapeOf support';
  }
  else
  {

    var s1 = evalcx('lazy');
    var s2 = evalcx('lazy');

    expect = shapeOf(s1);
    actual = shapeOf(s2);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
