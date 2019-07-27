




function check(obj, name, value, readonly) {
    
    
    var pd = Object.getOwnPropertyDescriptor(obj, name);

    assertEq(pd.configurable, false, "non-configurable " + name);
    assertEq(pd.writable, !readonly, "writable " + name);

    try {
        
        
        Object.defineProperty(obj, name, {writable: true});
        assertEq(0, 1);
    } catch (e) {
        assertEq('' + e, "TypeError: can't redefine non-configurable property '" + name + "'");
    }

    if (!readonly) {
        try {
            
            
            
            Object.defineProperty(obj, name, {value: value});
            assertEq(0, 1);
        } catch (e) {
            assertEq('' + e, "TypeError: can't redefine non-configurable property '" + name + "'");
        }
    }

    try {
        
        Object.defineProperty(obj, name, {value: "bogus", writable: false});
        assertEq(0, 1);
    } catch (e) {
        assertEq('' + e, "TypeError: can't redefine non-configurable property '" + name + "'");
    }

    
    Object.defineProperty(obj, name, {writable: false})

    
    assertEq(obj[name], value);

    
    obj[name] = "eek!";
    assertEq(obj[name], value);
}


/x/.test('x');

var d = Object.getOwnPropertyDescriptor(RegExp, "leftContext");
assertEq(d.set, undefined);
assertEq(typeof d.get, "function");
assertEq(d.enumerable, true);
assertEq(d.configurable, false);
assertEq(d.get.call(RegExp), "");

reportCompare(0, 0, "ok");
