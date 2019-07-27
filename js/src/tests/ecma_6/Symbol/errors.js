


if (typeof Symbol === "function") {
    

    var sym = Symbol();

    
    assertThrowsInstanceOf(() => sym(), TypeError);
    assertThrowsInstanceOf(() => Function.prototype.call.call(sym), TypeError);

    
    assertThrowsInstanceOf(() => new sym(), TypeError);
    assertThrowsInstanceOf(() => new Symbol(), TypeError);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
