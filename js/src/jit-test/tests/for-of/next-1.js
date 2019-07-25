

load(libdir + "asserts.js");
for (var v of [null, undefined, false, 0, "ponies", {}, [], this])
    assertThrowsInstanceOf(function () { Iterator.prototype.next.call(v); }, TypeError);
