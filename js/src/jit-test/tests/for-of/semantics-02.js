

load(libdir + "asserts.js");
function test(v) {
    Array.prototype.iterator = v;
    assertThrowsInstanceOf(function () { for (var x of []) ; }, TypeError);
}
test(undefined);
test(null);
test({});
