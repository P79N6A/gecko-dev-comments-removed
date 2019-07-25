






































var BUGNUMBER = 665286;
var summary = 'yield in comprehension RHS';
var actual = '';
var expect = '';

function reported() {
    [1 for (x in yield)]
}

reportCompare(reported.isGenerator(), true, "reported case: is generator");
reportCompare(typeof reported(), "object", "reported case: calling doesn't crash");
