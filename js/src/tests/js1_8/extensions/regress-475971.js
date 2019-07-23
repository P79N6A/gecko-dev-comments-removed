




































var gTestfile = 'regress-475971.js';

var BUGNUMBER = 475971;
var summary = 'js_CheckRedeclaration should unlock object on failures';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof scatter != 'function')
  {
    print(expect = actual = 
          'Test skipped - requires threaded build with scatter');
  }
  else
  {
    function x() { return 1; };

    
    
    function g()
    {
      var sum = 0;
      try {
        for (var i = 0; i != 10000; ++i) {
          sum += x();
        }
      } catch (e) { }
    }

    scatter([g, g]);

    try {
      eval("const x = 1");
    } catch (e) { }

    scatter([g, g]);

    print("Done");
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
