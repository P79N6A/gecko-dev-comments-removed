


function checkNotWritable(obj) {
    
    obj[sym] = "portals";
    assertEq(obj[sym], "cheese");

    
    assertThrowsInstanceOf(function () { "use strict"; obj[sym] = "robots"; }, TypeError);
    assertEq(obj[sym], "cheese");
}

if (typeof Symbol === "function") {
    var sym = Symbol.for("moon");

    var x = {};
    Object.defineProperty(x, sym, {
        configurable: true,
        enumerable: true,
        value: "cheese",
        writable: false
    });

    checkNotWritable(x);

    
    var y = Object.create(x);
    checkNotWritable(y);
    checkNotWritable(Object.create(y));
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
