





































var BUGNUMBER = 476871;
var summary = 'Do not assert: *(JSObject**)slot == NULL';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

let ([] = false) { (this.watch("x", /a/g)); };
(function () { (eval("(function(){for each (x in [1, 2, 2]);});"))(); })();

jit(false);

reportCompare(expect, actual, summary);
