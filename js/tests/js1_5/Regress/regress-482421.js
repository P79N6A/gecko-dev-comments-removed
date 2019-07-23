




































var gTestfile = 'regress-482421.js';

var BUGNUMBER = 482421;
var summary = 'TM: Do not assert: vp >= StackBase(fp)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

function f()
{
  var x;
  var foo = "for (var z = 0; z < 2; ++z) { new Object(new String(this), x)}";
  foo.replace(/\n/g, " ");
  eval(foo);
}
f();

jit(false);

reportCompare(expect, actual, summary);
