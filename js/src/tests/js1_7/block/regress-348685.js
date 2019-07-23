




































var gTestfile = 'regress-348685.js';

var BUGNUMBER = 348685;
var summary = 'Let scoped variables should not be referenced outside blocks';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function f() {
    for (let i = 0; i < 2; i++) {
      let j = 42;
      function g() {}
    }
    var a = i;
    print(a);
    print(i);
    return i;
  }

  expect = /ReferenceError: (i|"i") is not defined/;

  try
  {
    f();
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
