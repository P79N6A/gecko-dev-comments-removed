



assertThrowsInstanceOf(() => Array.from(), TypeError);
assertThrowsInstanceOf(() => Array.from(undefined), TypeError);
assertThrowsInstanceOf(() => Array.from(null), TypeError);


function ObjectWithReadOnlyElement() {
    Object.defineProperty(this, "0", {value: null});
    this.length = 0;
}
ObjectWithReadOnlyElement.from = Array.from;
assertDeepEq(ObjectWithReadOnlyElement.from([]), new ObjectWithReadOnlyElement);
assertThrowsInstanceOf(() => ObjectWithReadOnlyElement.from([1]), TypeError);


function InextensibleObject() {
    Object.preventExtensions(this);
}
InextensibleObject.from = Array.from;
assertThrowsInstanceOf(() => InextensibleObject.from([1]), TypeError);



var obj;
function init(self) {
    obj = self;
    self[0] = self[1] = self[2] = self[3] = 0;
}

function testUnsettableLength(C, Exc) {
    if (Exc === undefined)
        Exc = TypeError;  
    C.from = Array.from;

    obj = null;
    assertThrowsInstanceOf(() => C.from([]), Exc);
    assertEq(obj instanceof C, true);
    for (var i = 0; i < 4; i++)
        assertEq(obj[0], 0);

    obj = null;
    assertThrowsInstanceOf(() => C.from([0, 10, 20, 30]), Exc);
    assertEq(obj instanceof C, true);
    for (var i = 0; i < 4; i++)
        assertEq(obj[i], i * 10);
}



function InextensibleObject4() {
    init(this);
    Object.preventExtensions(this);
}
testUnsettableLength(InextensibleObject4);



function ObjectWithReadOnlyLength() {
    init(this);
    Object.defineProperty(this, "length", {configurable: true, writable: false, value: 4});
}
testUnsettableLength(ObjectWithReadOnlyLength);


Uint8Array.from = Array.from;
assertThrowsInstanceOf(() => Uint8Array.from([]), TypeError);



function ObjectWithInheritedReadOnlyLength() {
    init(this);
}
Object.defineProperty(ObjectWithInheritedReadOnlyLength.prototype,
                      "length",
                      {configurable: true, writable: false, value: 4});
testUnsettableLength(ObjectWithInheritedReadOnlyLength);


function ObjectWithGetterOnlyLength() {
    init(this);
    Object.defineProperty(this, "length", {configurable: true, get: () => 4});
}
testUnsettableLength(ObjectWithGetterOnlyLength);


function ObjectWithThrowingLengthSetter() {
    init(this);
    Object.defineProperty(this, "length", {
        configurable: true,
        get: () => 4,
        set: () => { throw new RangeError("surprise!"); }
    });
}
testUnsettableLength(ObjectWithThrowingLengthSetter, RangeError);


assertThrowsInstanceOf(() => Array.from([3, 4, 5], {}), TypeError);
assertThrowsInstanceOf(() => Array.from([3, 4, 5], "also not a function"), TypeError);
assertThrowsInstanceOf(() => Array.from([3, 4, 5], null), TypeError);


assertThrowsInstanceOf(() => Array.from([], JSON), TypeError);



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
assertThrowsInstanceOf(() => Array.from.call(C, p, {}), TypeError);
assertEq(log, "");


var arrayish = {
    get length() { log += "l"; return 1; },
    get 0() { log += "0"; return "q"; }
};
log = "";
var exc = {surprise: "ponies"};
assertThrowsValue(() => Array.from.call(C, arrayish, () => { throw exc; }), exc);
assertEq(log, "lC0");
assertEq(obj instanceof C, true);


for (var primitive of [undefined, null, 17]) {
    assertThrowsInstanceOf(
        () => Array.from({
            "@@iterator": () => {
                next: () => primitive
            }
        }),
        TypeError);
}

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
