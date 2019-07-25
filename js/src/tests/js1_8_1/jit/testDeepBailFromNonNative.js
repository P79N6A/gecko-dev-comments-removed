





































var BUGNUMBER = 0;
var summary = 'Test deep bail from non-native call; don\'t crash';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

test();

function test()
{
    try {
        [1 for each (i in this)];
    } catch (ex) {}
}

jit(false);
reportCompare(expect, actual, summary);
