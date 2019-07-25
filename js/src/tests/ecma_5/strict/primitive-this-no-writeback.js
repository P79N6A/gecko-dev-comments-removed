








var f = String.prototype.m = function () {
    "use strict";
    assertEq(this, "s");
    
    return [this.m, this];
};
var a = "s".m();
assertEq(a[0], f);
assertEq(a[1], "s");

reportCompare(true, true);
