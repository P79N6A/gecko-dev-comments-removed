

load(libdir + "asserts.js");

var p = Proxy.create({
    getPropertyDescriptor: function (name) {
        if (name == "iterator")
            throw "fit";
        return undefined;
    }
});
assertThrowsValue(function () { for (var v of p) {} }, "fit");
