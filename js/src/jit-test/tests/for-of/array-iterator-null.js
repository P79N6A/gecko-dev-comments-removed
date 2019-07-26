

load(libdir + "asserts.js");
load(libdir + "iteration.js");

for (var v of [undefined, null]) {
    
    assertThrowsInstanceOf(function () { Array.prototype[std_iterator].call(v); }, TypeError);
}
