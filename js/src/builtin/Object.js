




function ObjectStaticAssign(target, firstSource) {
    
    var to = ToObject(target);

    
    if (arguments.length < 2)
        return to;

    
    var i = 1;
    do {
        
        
        
        
        var nextSource = arguments[i];
        if (nextSource === null || nextSource === undefined)
            continue;

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
    } while (++i < arguments.length);

    
    return to;
}

function ObjectDefineSetter(name, setter) {
    var object;
    if (this === null || this === undefined)
        object = global;
    else
        object = ToObject(this);

    if (!IsCallable(setter))
        ThrowError(JSMSG_BAD_GETTER_OR_SETTER, "setter");

    var key = ToPropertyKey(name);

    var desc = {
        __proto__: null,
        enumerable: true,
        configurable: true,
        set: setter
    };

    std_Object_defineProperty(object, key, desc);
}

function ObjectDefineGetter(name, getter) {
    var object;
    if (this === null || this === undefined)
        object = global;
    else
        object = ToObject(this);

    if (!IsCallable(getter))
        ThrowError(JSMSG_BAD_GETTER_OR_SETTER, "getter");

    var key = ToPropertyKey(name);

    var desc = {
        __proto__: null,
        enumerable: true,
        configurable: true,
        get: getter
    };

    std_Object_defineProperty(object, key, desc);
}

function ObjectLookupSetter(name) {
    var key = ToPropertyKey(name);
    var object = ToObject(this);

    do {
        var desc = std_Object_getOwnPropertyDescriptor(object, key);
        if (desc) {
            if (callFunction(std_Object_hasOwnProperty, desc, "set"))
                return desc.set;
            return undefined;
        }
        object = std_Object_getPrototypeOf(object);
    } while (object !== null);
}

function ObjectLookupGetter(name) {
    var key = ToPropertyKey(name);
    var object = ToObject(this);

    do {
        var desc = std_Object_getOwnPropertyDescriptor(object, key);
        if (desc) {
            if (callFunction(std_Object_hasOwnProperty, desc, "get"))
                return desc.get;
            return undefined;
        }
        object = std_Object_getPrototypeOf(object);
    } while (object !== null);
}
