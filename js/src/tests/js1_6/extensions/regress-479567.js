





































var BUGNUMBER = 479567;
var summary = 'Do not assert: thing';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

function f()
{
  (eval("(function(){for each (y in [false, false, false]);});"))();
}

try
{
  this.__defineGetter__("x", gc);
  uneval(this.watch("y", this.toSource))
    f();
  throw x;
}
catch(ex)
{
}

jit(false);

reportCompare(expect, actual, summary);
