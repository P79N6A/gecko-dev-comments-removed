


if (typeof Symbol === "function") {
    

    var symbols = [
        Symbol(),
        Symbol.for("ponies"),
        Symbol.iterator
    ];

    for (var sym of symbols) {
        assertEq(JSON.stringify(sym), undefined);
        assertEq(JSON.stringify([sym]), "[null]");

        
        assertEq(JSON.stringify({x: sym}), '{}');

        
        var replacer = function (key, val) {
            assertEq(typeof this, "object");
            if (typeof val === "symbol") {
                assertEq(val, sym);
                return "ding";
            }
            return val;
        };
        assertEq(JSON.stringify(sym, replacer), '"ding"');
        assertEq(JSON.stringify({x: sym}, replacer), '{"x":"ding"}');
    }
}

if (typeof reportCompare === 'function')
    reportCompare(0, 0, 'ok');
