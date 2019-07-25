




function strictThis() { 'use strict'; return this; }


function flat(g) {
    function h() { return g(); }
    return h;
}
assertEq(flat(strictThis)(), undefined);


function upvar(f) {
    function h() {
        return f(); 
    }
    return h();
}
assertEq(upvar(strictThis), undefined);


var obj = { f: strictThis };
with (obj) {
    



    function g() { return f(); }
    assertEq(g(), obj);
}

reportCompare(true, true);
