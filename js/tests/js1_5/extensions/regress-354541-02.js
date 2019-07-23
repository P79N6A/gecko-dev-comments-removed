





































var gTestfile = 'regress-354541-02.js';

var BUGNUMBER = 354541;
var summary = 'Regression to standard class constructors in case labels';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary + ': in function');

  String.prototype.trim = function() { print('hallo'); };

  const S = String;
  const Sp = String.prototype;

  expect = 'No Error';
  actual = 'No Error';
  if (typeof Script == 'undefined')
  {
    print('Test skipped. Script not defined.');
  }
  else
  {
    var s = Script('var tmp = function(o) { switch(o) { case String: case 1: return ""; } }; print(String === S); print(String.prototype === Sp); "".trim();');
    s();
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
