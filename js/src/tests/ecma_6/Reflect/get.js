




var x = {p: 1};
assertEq(Reflect.get(x, "p"), 1);
assertEq(Reflect.get(x, "toString"), Object.prototype.toString);
assertEq(Reflect.get(x, "missing"), undefined);





assertEq(Reflect.get([], 700), undefined);
assertEq(Reflect.get(["zero", "one"], 1), "one");


assertEq(Reflect.get(new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7]), 7), 7);


var f = new Float64Array([NaN]);
var u = new Uint32Array(f.buffer);
u[0]++;
u[1]++;
assertEq(f[0], NaN);
assertEq(Reflect.get(f, 0), NaN);


assertEq(Reflect.get(new Proxy(x, {}), "p"), 1);


var obj = new Proxy(x, {
    get(t, k, r) { return k + "ful"; }
});
assertEq(Reflect.get(obj, "mood"), "moodful");


assertThrowsInstanceOf(() => Reflect.get(obj, Symbol()), TypeError);


obj = {set name(x) {}};
assertEq(Reflect.get(obj, "name"), undefined);





obj = { get x() { return this; }};
assertEq(Reflect.get(obj, "x", Math), Math);
assertEq(Reflect.get(Object.create(obj), "x", JSON), JSON);


assertEq(Reflect.get(obj, "x"), obj);


obj = new Proxy({}, {
    get(t, k, r) {
        assertEq(k, "itself");
        return r;
    }
});
assertEq(Reflect.get(obj, "itself"), obj);
assertEq(Reflect.get(obj, "itself", Math), Math);
assertEq(Reflect.get(Object.create(obj), "itself", Math), Math);




var result;
try {
    result = Reflect.get(obj, "x", 37.2);
} catch (exc) {
    result = exc;
}
if (result === 37.2) {
    throw new Error("Congratulations on fixing bug 603201! " +
                    "Please update this test for 1 karma point.");
}
assertEq(result instanceof TypeError, true);




reportCompare(0, 0);
