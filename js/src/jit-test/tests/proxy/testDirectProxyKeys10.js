load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, "foo", { configurable: true, enumerable: false });

function getPD(target, P) {
    var targetDesc = Object.getOwnPropertyDescriptor(target, P);
    
    targetDesc.enumerable = !targetDesc.enumerable;
    return targetDesc;
}

var proxy = new Proxy(target, { getOwnPropertyDescriptor: getPD });

assertDeepEq(Object.keys(proxy), ["foo"]);

Object.defineProperty(target, "foo", {configurable: true, enumerable: true});

assertDeepEq(Object.keys(proxy), []);
