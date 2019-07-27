const constructors = [
    Int8Array,
    Uint8Array,
    Uint8ClampedArray,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Float32Array,
    Float64Array
];

for (var constructor of constructors) {
    
    assertThrowsInstanceOf(() => constructor.from(), TypeError);
    assertThrowsInstanceOf(() => constructor.from(undefined), TypeError);
    assertThrowsInstanceOf(() => constructor.from(null), TypeError);

    
    function ObjectWithReadOnlyElement() {
        Object.defineProperty(this, "0", {value: null});
        this.length = 0;
    }
    ObjectWithReadOnlyElement.from = constructor.from;
    assertDeepEq(ObjectWithReadOnlyElement.from([]), new ObjectWithReadOnlyElement);
    assertThrowsInstanceOf(() => ObjectWithReadOnlyElement.from([1]), TypeError);

    
    function InextensibleObject() {
        Object.preventExtensions(this);
    }
    InextensibleObject.from = constructor.from;
    assertThrowsInstanceOf(() => InextensibleObject.from([1]), TypeError);

    
    function ObjectWithReadOnlyElementOnProto() {
        return Object.create({
            get 0(){}
        });
    }
    ObjectWithReadOnlyElementOnProto.from = constructor.from;
    assertThrowsInstanceOf(() => ObjectWithReadOnlyElementOnProto.from([1]), TypeError);

    
    function ObjectWithThrowingLengthGetterSetter() {
        Object.defineProperty(this, "length", {
            configurable: true,
            get() { throw new RangeError("getter!"); },
            set() { throw new RangeError("setter!"); }
        });
    }
    ObjectWithThrowingLengthGetterSetter.from = constructor.from;
    assertEq(ObjectWithThrowingLengthGetterSetter.from(["foo"])[0], "foo");

    
    assertThrowsInstanceOf(() => constructor.from([3, 4, 5], {}), TypeError);
    assertThrowsInstanceOf(() => constructor.from([3, 4, 5], "also not a function"), TypeError);
    assertThrowsInstanceOf(() => constructor.from([3, 4, 5], null), TypeError);

    
    assertThrowsInstanceOf(() => constructor.from([], JSON), TypeError);

    
    
    var log = "";
    function C() {
        log += "C";
        obj = this;
    }
    var p = new Proxy({}, {
        has: function () { log += "1"; },
        get: function () { log += "2"; },
        getOwnPropertyDescriptor: function () { log += "3"; }
    });
    assertThrowsInstanceOf(() => constructor.from.call(C, p, {}), TypeError);
    assertEq(log, "");

    
    var arrayish = {
        get length() { log += "l"; return 1; },
        get 0() { log += "0"; return "q"; }
    };
    log = "";
    var exc = {surprise: "ponies"};
    assertThrowsValue(() => constructor.from.call(C, arrayish, () => { throw exc; }), exc);
    assertEq(log, "lC0");
    assertEq(obj instanceof C, true);

    
    for (var primitive of ["foo", 17, Symbol(), true]) {
        assertThrowsInstanceOf(() => constructor.from({[Symbol.iterator] : primitive}), TypeError);
    }
    assertDeepEq(constructor.from({[Symbol.iterator]: null}), new constructor());
    assertDeepEq(constructor.from({[Symbol.iterator]: undefined}), new constructor());

    
    for (var primitive of [undefined, null, "foo", 17, Symbol(), true]) {
        assertThrowsInstanceOf(
            () => constructor.from({
                [Symbol.iterator]() {
                    return {next() { return primitive; }};
                }
            }),
        TypeError);
    }
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
