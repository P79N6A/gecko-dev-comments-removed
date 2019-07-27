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
    
    
    
    var d = constructor.from.call(Date, ["A", "B"]);
    assertEq(d instanceof constructor, false);
    assertEq(Object.prototype.toString.call(d), "[object Date]");
    assertEq(Object.getPrototypeOf(d), Date.prototype);
    assertEq(d.length, undefined);
    assertEq(d[0], "A");
    assertEq(d[1], "B");

    
    var obj = constructor.from.call(RegExp, [1]);
    assertEq(obj instanceof constructor, false);
    assertEq(Object.getPrototypeOf(obj), RegExp.prototype);
    assertEq(Object.getOwnPropertyNames(obj).join(","),
             "0,lastIndex");
    assertEq(obj.length, undefined);

    
    function C(arg) {
        this.args = arguments;
    }
    var c = constructor.from.call(C, {length: 1, 0: "zero"});
    assertEq(c instanceof C, true);
    assertEq(c.args.length, 1);
    assertEq(c.args[0], 1);
    assertEq(c.length, undefined);
    assertEq(c[0], "zero");

    
    
    assertEq(constructor.from.call(Object, []) instanceof Number, true);
    assertDeepEq(constructor.from.call(Object, []), new Number(0));
    assertEq(constructor.from.call(Number,[1, , "a"]) + 1, 4);
    constructor.from.call(function(len){
        assertEq(len, 3);
    }, Array(3));

    
    
    var arr = [3, 4, 5];
    var nonconstructors = [
        {}, Math, Object.getPrototypeOf, undefined, 17,
        () => ({})  
    ];
    for (var v of nonconstructors) {
        assertThrowsInstanceOf(() => {
            constructor.from.call(v, arr);
        }, TypeError);
    }

    
    
    function NotArray() {
    }
    var RealArray = constructor;
    NotArray.from = constructor.from;
    this[constructor.name] = NotArray;
    assertEq(RealArray.from([1]) instanceof RealArray, true);
    assertEq(NotArray.from([1]) instanceof NotArray, true);
    this[constructor.name] = RealArray;
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
