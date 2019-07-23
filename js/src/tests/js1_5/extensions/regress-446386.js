




































var gTestfile = 'regress-446386.js';

var BUGNUMBER = 446386;
var summary = 'Do not crash throwing error without compiler pseudo-frame';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof evalcx == 'undefined')
  {
    print(expect = actual = 'Test skipped. evalcx required.');
  }
  else {
    try
    {
      try {
	evalcx(".");
	throw "must throw";
      } catch (e) {
	if (e.name != "SyntaxError")
	  throw e;
      }
    }
    catch(ex)
    {
      actual = ex + '';
    }
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
