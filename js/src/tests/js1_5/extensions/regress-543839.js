






var gTestfile = 'regress-543839.js';

var BUGNUMBER = 543839;
var summary = 'js_GetMutableScope caller must lock the object';
var actual;
var expect = 1;

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

function test()
{
    jit(true);
    for (var i = 0; i != 100; ++i)
        var tmp = { a: 1 };
    return 1;
}

test();
test();
test();
actual = evalcx("test()", this);

reportCompare(expect, actual, summary);
