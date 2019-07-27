


if (typeof Symbol === "function") {
    assertEq(typeof Symbol(), "symbol");
    assertEq(typeof Symbol("ponies"), "symbol");
    assertEq(typeof Symbol.for("ponies"), "symbol");

    assertEq(typeof Object(Symbol()), "object");
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
