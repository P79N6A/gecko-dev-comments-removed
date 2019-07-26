


assertEq(typeof Symbol(), "symbol");
assertEq(typeof Symbol("ponies"), "symbol");
assertEq(typeof Symbol.for("ponies"), "symbol");

if (typeof reportCompare === "function")
    reportCompare(0, 0);
