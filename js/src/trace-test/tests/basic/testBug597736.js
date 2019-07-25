function leak_test() {
    
    
    
    

    
    
    
    for (var j = 0; j != 8; ++j) {
	var f = Function("a", "var s = 0; for (var i = 0; i != 100; ++i) s += a.b; return s;");
	var c = {b: 1, f: f, leakDetection: makeFinalizeObserver()};
	f({ __proto__: { __proto__: c}});
	f = c = a = null;
	gc();
    }
}

function test()
{
    if (typeof finalizeCount != "function")
	return;

    var base = finalizeCount();
    leak_test();
    gc();
    gc();
    var n = finalizeCount();
    assertEq(base + 4 < finalizeCount(), true, "Some finalizations must happen");
}

test();
