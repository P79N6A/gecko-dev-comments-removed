


if (typeof Symbol === "function") {
    
    
    
    
    

    var symbols = [
        Symbol(),
        Symbol("ponies"),
        Symbol.for("sym"),
        Symbol.iterator
    ];

    
    var gets, sets;
    Object.defineProperty(Symbol.prototype, "prop", {
        get: function () {
            "use strict";
            gets++;
            assertEq(typeof this, "object");
            assertEq(this instanceof Symbol, true);
            assertEq(this.valueOf(), sym);
            return "got";
        },
        set: function (v) {
            "use strict";
            sets++;
            assertEq(typeof this, "object");
            assertEq(this instanceof Symbol, true);
            assertEq(this.valueOf(), sym);
            assertEq(v, "newvalue");
        }
    });

    for (var sym of symbols) {
        assertEq(sym.constructor, Symbol);

        
        assertEq(sym.hasOwnProperty("constructor"), false);
        assertEq(sym.toLocaleString(), sym.toString()); 

        
        Symbol.prototype.nonStrictMethod = function (arg) {
            assertEq(arg, "ok");
            assertEq(this instanceof Symbol, true);
            assertEq(this.valueOf(), sym);
            return 13;
        };
        assertEq(sym.nonStrictMethod("ok"), 13);

        
        Symbol.prototype.strictMethod = function (arg) {
            "use strict";
            assertEq(arg, "ok2");
            assertEq(this, sym);
            return 14;
        };
        assertEq(sym.strictMethod("ok2"), 14);

        
        gets = 0;
        sets = 0;
        var propname = "prop";

        assertEq(sym.prop, "got");
        assertEq(gets, 1);
        assertEq(sym[propname], "got");
        assertEq(gets, 2);

        assertEq(sym.prop = "newvalue", "newvalue");
        assertEq(sets, 1);
        assertEq(sym[propname] = "newvalue", "newvalue");
        assertEq(sets, 2);

        
        assertEq(sym.noSuchProp, undefined);
        var noSuchPropName = "nonesuch";
        assertEq(sym[noSuchPropName], undefined);

        
        assertThrowsInstanceOf(() => sym.noSuchProp(), TypeError);
        assertThrowsInstanceOf(() => sym[noSuchPropName](), TypeError);
    }
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
