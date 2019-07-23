




































var gTestfile = 'regress-406555.js';

var BUGNUMBER = 406555;
var summary = 'decompiler should not depend on JS_C_STRINGS_ARE_UTF8';
var actual;
var expect;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function() {return "\uD800\uD800";});
  var g = uneval(f);
  var h = eval(g);

  expect = "\uD800\uD800";
  actual = h();
  reportCompare(expect, actual, summary + ': h() == \\uD800\\uD800');

  expect = g;
  actual = uneval(h);
  reportCompare(expect, actual, summary + ': g == uneval(h)');

  exitFunc ('test');
}
