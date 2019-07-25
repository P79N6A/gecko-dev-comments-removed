

load(libdir + "asserts.js");
for (var v of [undefined, null]) {
    var it = Array.prototype.iterator.call(v);

    
    assertThrowsInstanceOf(function () { it.next(); }, TypeError);
}
