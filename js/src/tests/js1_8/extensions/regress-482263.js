




































var gTestfile = 'regress-482263.js';

var BUGNUMBER = 482263;
var summary = 'TM: Do not assert: x->oprnd2() == lirbuf->sp || x->oprnd2() == gp_ins';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

__proto__.x getter= function () { return <y/>.([]) };
for each (let x in []) { for each (let x in ['', '']) { } }

jit(true);

reportCompare(expect, actual, summary);

delete __proto__.x;
