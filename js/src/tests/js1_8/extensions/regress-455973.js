





var BUGNUMBER = 455973;
var summary = 'Do not assert: !cx->throwing';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

try
{
  for (let i = 0; i < 5; ++i) void (this["y" + i] = "");
  this.__defineGetter__("z", function () { throw 2; });
  for (let j = 0; j < 2; ++j) { [1 for each (q in this) if ('')]; } 
}
catch(ex)
{
}

jit(false);

reportCompare(expect, actual, summary);
