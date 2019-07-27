


var symbols = [
    Symbol(),
    Symbol("one"),
    Symbol.for("two"),
    Symbol.iterator,
    Object(Symbol())
];

for (var sym of symbols) {
    var obj = {};

    
    assertEq(sym in obj, false);
    assertEq(obj.hasOwnProperty(sym), false);
    assertEq(obj[sym], undefined);
    assertEq(typeof obj[sym], "undefined");
    assertEq(Object.getOwnPropertyDescriptor(obj, sym), undefined);

    
    obj[sym] = "ok";
    assertEq(sym in obj, true);
    assertEq(obj.hasOwnProperty(sym), true);
    assertEq(obj[sym], "ok");
    assertDeepEq(Object.getOwnPropertyDescriptor(obj, sym), {
        value: "ok",
        writable: true,
        enumerable: true,
        configurable: true
    });

    
    obj[sym] = 12;
    assertEq(obj[sym], 12);

    
    assertEq(obj[sym]++, 12);
    assertEq(obj[sym], 13);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
