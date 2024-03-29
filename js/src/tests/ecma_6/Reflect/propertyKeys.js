














var keys = [
    {value: null, expected: "null"},
    {value: undefined, expected: "undefined"},
    {value: true, expected: "true"},
    {value: 42, expected: "42"},
    {value: "string"},
    {value: ""},
    {value: "string with \0"},
    {value: new String("ok"), expected: "ok"},
    {value: Symbol("sym")},
    {value: Symbol.iterator},
    {value: Object(Symbol.for("comet")), expected: Symbol.for("comet")},
    {
        value: {
            toString() { return "key"; },
            valueOf() { return "bad"; }
        },
        expected: "key"
    },
    {
        value: {
            toString: undefined,
            valueOf() { return "fallback"; }
        },
        expected: "fallback"
    }
];

if ("toPrimitive" in Symbol) {
    throw new Error("Congratulations on implementing Symbol.toPrimitive! " +
                    "Please add an object with an @@toPrimitive method in the list above.");
}

for (var {value, expected} of keys) {
    if (expected === undefined)
        expected = value;

    var obj = {};
    assertEq(Reflect.defineProperty(obj, value, {value: 1, configurable: true}), true);
    assertDeepEq(Reflect.ownKeys(obj), [expected]);
    assertDeepEq(Reflect.getOwnPropertyDescriptor(obj, value),
                 {value: 1,
                  writable: false,
                  enumerable: false,
                  configurable: true});
    assertEq(Reflect.deleteProperty(obj, value), true);
    assertEq(Reflect.has(obj, value), false);
    assertEq(Reflect.set(obj, value, 113), true);
    assertEq(obj[expected], 113);
    assertEq(Reflect.has(obj, value), true);
    assertEq(Reflect.get(obj, value), 113);
}


var exc = {};
var badKey = {toString() { throw exc; }};
var methodNames = ["defineProperty", "deleteProperty", "has", "get", "getOwnPropertyDescriptor", "set"];
for (var name of methodNames) {
    assertThrowsValue(() => Reflect[name]({}, badKey), exc);
}

reportCompare(0, 0);
