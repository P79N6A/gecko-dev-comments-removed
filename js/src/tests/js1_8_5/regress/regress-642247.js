





if (typeof timeout == "function") {
    assertEq(typeof timeout(), "number");
    assertEq(typeof timeout(1), "undefined");
}

reportCompare(0, 0, "ok");

