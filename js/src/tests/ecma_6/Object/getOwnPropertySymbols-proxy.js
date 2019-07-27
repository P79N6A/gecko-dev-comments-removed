


function HandlerProxy() {
    return new Proxy({}, {
        get: function (t, key) {
            if (key !== "ownKeys")
                throw new Error("tried to access handler[" + uneval(key) + "]");
            hits++;
            return t => symbols;
        }
    });
}

function OwnKeysProxy() {
    return new Proxy({}, new HandlerProxy);
}

if (typeof Symbol === "function") {
    

    var symbols = [Symbol(), Symbol("moon"), Symbol.for("sun"), Symbol.iterator];
    var hits = 0;

    assertDeepEq(Object.getOwnPropertySymbols(new OwnKeysProxy), symbols);
    assertEq(hits, 1);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
