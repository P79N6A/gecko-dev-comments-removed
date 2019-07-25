

load(libdir + "asserts.js");

var p = Proxy.create({
    iterate: function () { throw "FAIL"; },
    fix: function () { return {}; }
});
Object.preventExtensions(p);
assertThrowsInstanceOf(function () { for (var v of p) {} }, TypeError);
