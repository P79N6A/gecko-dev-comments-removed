




































var gTestfile = 'regress-453747.js';

var BUGNUMBER = 453747;
var summary = 'Do not assert with JIT: JSVAL_IS_VOID(boxed) || JSVAL_IS_BOOLEAN(boxed)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  (function(){
    var a = [];
    var s = 10;
    for (var i = 0; i < s; ++i)
      a[i] = 1;
    a[4*s-1] = 2;
    for (var i = s+1; i < s+4; ++i)
      typeof a[i];
  })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
