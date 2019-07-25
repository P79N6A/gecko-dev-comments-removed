












































function deepEqual(a, b) {
    if (typeof a != typeof b)
        return false;

    if (typeof a == 'object') {
        var props = {};
        
        for (var prop in a) {
            if (!deepEqual(a[prop], b[prop]))
                return false;
            props[prop] = true;
        }
        
        for (var prop in b)
            if (!props[prop])
                return false;
        
        return a.length == b.length;
    }

    if (a === b) {
        
        return a !== 0 || 1/a === 1/b;
    }

    
    
    
    return a !== a && b !== b;
}
