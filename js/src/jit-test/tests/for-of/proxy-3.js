

load(libdir + "asserts.js");
load(libdir + "iteration.js");

var p = Proxy.create({
    getPropertyDescriptor: function (name) {
        if (name == std_iterator)
            throw "fit";
        return undefined;
    }
});
assertThrowsValue(function () { for (var v of p) {} }, "fit");
