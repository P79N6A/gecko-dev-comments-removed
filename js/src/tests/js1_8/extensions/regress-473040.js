





































var BUGNUMBER = 473040;
var summary = 'Do not assert: tm->reservedDoublePoolPtr > tm->reservedDoublePool';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

Object.defineProperty(__proto__, "functional",
{
  enumerable: true, configurable: true,
  get: new Function("gc()")
});
for each (let x in [new Boolean(true), new Boolean(true), -0, new
                    Boolean(true), -0]) { undefined; }

jit(false);

reportCompare(expect, actual, summary);
