




































var gTestfile = 'regress-466905-04.js';

var BUGNUMBER = 466905;
var summary = 'Prototypes of sandboxed arrays';
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
  else
  {
    expect = true;

    function createArray()
    {
      var a;                
      for (var i = 0; i < 10; i++)
        a = [1, 2, 3, 4, 5];
      return a;
    }

    var sandbox = evalcx("lazy");
    sandbox.createArray = createArray;
    var p1 = Object.getPrototypeOf(createArray());
    var p2 = Object.getPrototypeOf(evalcx("createArray()", sandbox));
    print(actual = (p1 === p2));
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
