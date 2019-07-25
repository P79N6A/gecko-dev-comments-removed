




function check(obj, name, value, readonly) {
    
    
    var pd = Object.getOwnPropertyDescriptor(obj, name);

    assertEq(pd.configurable, false, "non-configurable " + name);
    assertEq(pd.writable, !readonly, "writable " + name);

    try {
        
        
        Object.defineProperty(obj, name, {writable: true});

        if (!readonly) {
            
            
            
            Object.defineProperty(obj, name, {value: value});
        }
    } catch (e) {
        assertEq('' + e, "TypeError: can't redefine non-configurable property '" + name + "'");
    }

    
    Object.defineProperty(obj, name, {writable: false})

    
    assertEq(obj[name], value);

    
    obj[name] = "eek!";
    assertEq(obj[name], value);
}

check(Object, 'caller', null, false);
check(Object, 'arguments', null, false);


/x/.test('x');
check(RegExp, 'leftContext', '', true);

reportCompare(0, 0, "ok");
