




































var gTestfile = 'regress-349616.js';

var BUGNUMBER = 349616;
var summary = 'decompilation of getter keyword';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;
  f = function() {
    window.foo getter = function() { return 5; };
    print(window.foo);
  }

  actual = f + '';
  expect = 'function () {\n    window.foo getter= ' +
    'function () {return 5;};\n    print(window.foo);\n}';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
