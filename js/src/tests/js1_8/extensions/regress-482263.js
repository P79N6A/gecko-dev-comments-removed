





var BUGNUMBER = 482263;
var summary = 'TM: Do not assert: x->oprnd2() == lirbuf->sp || x->oprnd2() == gp_ins';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

Object.defineProperty(__proto__, "x",
{
  enumerable: true, configurable: true,
  get: function () { return ([]) }
});
for each (let x in []) { for each (let x in ['', '']) { } }

jit(true);

reportCompare(expect, actual, summary);

delete __proto__.x;
