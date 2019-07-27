




function ObjectStaticAssign(target, firstSource) {
    
    var to = ToObject(target);

    
    if (arguments.length < 2)
        return to;

    
    var i = 1;
    do {
        
        var nextSource = arguments[i];
        var from = ToObject(nextSource);

        
        var keysArray = OwnPropertyKeys(from);

        
        var len = keysArray.length;

        
        var nextIndex = 0;

        
        
        const MISSING = {};
        var pendingException = MISSING;

        
        while (nextIndex < len) {
            
            var nextKey = keysArray[nextIndex];

            
            try {
                
                
                
                
                var desc = std_Object_getOwnPropertyDescriptor(from, nextKey);
                if (desc !== undefined && desc.enumerable)
                    to[nextKey] = from[nextKey];
            } catch (e) {
                if (pendingException === MISSING)
                    pendingException = e;
            }

            
            nextIndex++;
        }

        
        if (pendingException !== MISSING)
            throw pendingException;
        i++;
    } while (i < arguments.length);

    
    return to;
}

