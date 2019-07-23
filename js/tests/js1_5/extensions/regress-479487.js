




































var gTestfile = 'regress-479487.js';

var BUGNUMBER = 479487;
var summary = 'js_Array_dense_setelem can call arbitrary JS code';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  Array.prototype[1] = 2;

  Array.prototype.__defineSetter__(32, function() { print("Hello from arbitrary JS");});
  Array.prototype.__defineGetter__(32, function() { return 11; });

  function f()
  {
    var a = [];
    for (var i = 0; i != 10; ++i) {
      a[1 << i] = 9999;
    }
    return a;
  }

  f();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
