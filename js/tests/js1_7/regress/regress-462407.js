




































var gTestfile = 'regress-462407.js';

var BUGNUMBER = 462407;
var summary = 'Do not assert: !ti->stackTypeMap.matches(ti_other->stackTypeMap)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

(function f() { for each (let i in [0, {}, 0, 1.5, {}, 0, 1.5, 0, 0]) { }})();

jit(false);

reportCompare(expect, actual, summary);
