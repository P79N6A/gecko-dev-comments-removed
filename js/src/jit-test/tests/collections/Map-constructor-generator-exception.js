

load(libdir + "asserts.js");

function data2() {
    yield [{}, "XR22/Z"];
    yield [{}, "23D-BN"];
    throw "oops";
}

var it = data2();
assertThrowsValue(function () { Map(it); }, "oops");
