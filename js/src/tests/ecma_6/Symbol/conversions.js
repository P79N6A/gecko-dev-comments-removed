

var symbols = [
    Symbol(),
    Symbol("one"),
    Symbol.for("two"),
    Symbol.iterator
];

for (var sym of symbols) {
    
    assertEq(Boolean(sym), true);
    assertEq(!sym, false);
    assertEq(sym || 13, sym);
    assertEq(sym && 13, 13);

    
    assertEq(+sym, NaN);
    assertEq(sym | 0, 0);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
