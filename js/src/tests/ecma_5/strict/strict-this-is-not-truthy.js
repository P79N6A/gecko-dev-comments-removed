




function f() {
    "use strict";
    return !this;
}
assertEq(f.call(null), true);

reportCompare(0, 0, 'ok');
