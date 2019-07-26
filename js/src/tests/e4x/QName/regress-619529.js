







var b = Proxy.create({ enumerateOwn: function () { @f; }});
Object.freeze(this);

try {
    @r;
    throw 1; 
} catch (e) {
    assertEq(e instanceof ReferenceError, true);
}

reportCompare(0, 0, "ok");
