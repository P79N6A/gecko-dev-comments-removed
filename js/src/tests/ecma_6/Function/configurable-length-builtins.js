



for (var fun of [Math.sin, Array.prototype.map, eval]) {
    assertEq(delete fun.length, true);
    assertEq(fun.hasOwnProperty("length"), false);
    assertEq("length" in fun, true);  
    assertEq(fun.length, 0);

    
    
    fun.length = Math.hypot;
    assertEq(fun.length, 0);

    
    Object.defineProperty(fun, "length", {value: Math.hypot});
    assertEq(fun.length, Math.hypot);
}

reportCompare(0, 0, 'ok');
