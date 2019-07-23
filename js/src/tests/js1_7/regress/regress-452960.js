




































var gTestfile = 'regress-452960.js';

var BUGNUMBER = 452960;
var summary = 'Do not assert with JIT: !JSVAL_IS_PRIMITIVE(v)';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var f = function(){};
  f.prototype = false;
  for (let j=0;j<5;++j) { new f; }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
