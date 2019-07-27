





try {
    evalcx("Object.freeze(this); eval('const q = undefined;')");
} catch (e) {
    assertEq(e.message, "({lazy:false}) is not extensible");
}

reportCompare(0, 0, "don't crash");
