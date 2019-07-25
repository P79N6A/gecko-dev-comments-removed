






































var BUGNUMBER = 665286;
var summary = 'yield in arguments list';
var actual = '';
var expect = '';

function reported() {
    function f() {
        x
    }
    f(yield #2=[])
}

function simplified1() {
    print(yield)
}

function simplified2() {
    print(1, yield)
}

reportCompare(reported.isGenerator(), true, "reported case: is generator");
reportCompare(typeof reported(), "object", "reported case: calling doesn't crash");
reportCompare(simplified1.isGenerator(), true, "simplified case 1: is generator");
reportCompare(typeof simplified1(), "object", "simplified case 1: calling doesn't crash");
reportCompare(simplified2.isGenerator(), true, "simplified case 2: is generator");
reportCompare(typeof simplified2(), "object", "simplified case 2: calling doesn't crash");
