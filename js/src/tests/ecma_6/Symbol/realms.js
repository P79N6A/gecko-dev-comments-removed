




if (typeof Reflect !== "undefined" && typeof Reflect.Realm === "function") {
    throw new Error("Congratulations on implementing Reflect.Realm! " +
                    "Please update this test to use it.");
}
if (typeof newGlobal === "function") {
    var g = newGlobal();
    var gj = g.eval("jones = Symbol('jones')");
    assertEq(typeof gj, "symbol");
    assertEq(g.jones, g.jones);
    assertEq(gj, g.jones);
    assertEq(gj !== Symbol("jones"), true);

    
    
    var smith = Symbol("smith");
    g.smith = smith;  
    assertEq(g.smith, smith);  
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
