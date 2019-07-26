




load(libdir + "asserts.js");

if (typeof assertIteratorResult === 'undefined') {
    var assertIteratorResult = function assertIteratorResult(result, value, done) {
        assertEq(typeof result, "object");
        assertDeepEq(result.value, value);
        assertDeepEq(result.done, done);
    }
}
