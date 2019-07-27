load(libdir + "asserts.js");


var keys = ['foo'];
if (typeof Symbol === "function")
    keys.push(Symbol.for('quux'));
for (var key of keys) {
    var target = {};
    Object.defineProperty(target, key, {
        value: 'bar',
        writable: false,
        configurable: false
    });
    var handler = { set: () => true };
    for (let p of [new Proxy(target, handler), Proxy.revocable(target, handler).proxy])
        assertThrowsInstanceOf(() => p[key] = 'baz', TypeError);
}
