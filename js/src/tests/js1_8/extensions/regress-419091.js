






var BUGNUMBER = 419091;
var summary = 'Do not assert: JS_PROPERTY_CACHE(cx).disabled >= 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof scatter == 'undefined')
  {
    print(expect = actual = 'Test skipped. Requires scatter.');
  }
  else
  {
    if (typeof gczeal == 'undefined')
    {
      gczeal = (function () {});
    }

    gczeal(2);

    function f() {
      for (let i = 0; i < 10; i++) {
        let y = { x: i }
      }
    }

    for (let i = 0; i < 10; i++)
      scatter([f, f]);

    gczeal(0);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
