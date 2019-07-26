















function testBuiltInObject(obj, isFunction, isConstructor, properties, length) {

    if (obj === undefined) {
        $ERROR("Object being tested is undefined.");
    }

    var objString = Object.prototype.toString.call(obj);
    if (isFunction) {
        if (objString !== "[object Function]") {
            $ERROR("The [[Class]] internal property of a built-in function must be " +
                    "\"Function\", but toString() returns " + objString);
        }
    } else {
        if (objString !== "[object Object]") {
            $ERROR("The [[Class]] internal property of a built-in non-function object must be " +
                    "\"Object\", but toString() returns " + objString);
        }
    }

    if (!Object.isExtensible(obj)) {
        $ERROR("Built-in objects must be extensible.");
    }

    if (isFunction && Object.getPrototypeOf(obj) !== Function.prototype) {
        $ERROR("Built-in functions must have Function.prototype as their prototype.");
    }

    if (isConstructor && Object.getPrototypeOf(obj.prototype) !== Object.prototype) {
        $ERROR("Built-in prototype objects must have Object.prototype as their prototype.");
    }

    
    
    
    
    

    if (isFunction) {
        
        if (typeof obj.length !== "number" || obj.length !== Math.floor(obj.length)) {
            $ERROR("Built-in functions must have a length property with an integer value.");
        }
    
        if (obj.length !== length) {
            $ERROR("Function's length property doesn't have specified value; expected " +
                length + ", got " + obj.length + ".");
        }

        var desc = Object.getOwnPropertyDescriptor(obj, "length");
        if (desc.writable) {
            $ERROR("The length property of a built-in function must not be writable.");
        }
        if (desc.enumerable) {
            $ERROR("The length property of a built-in function must not be enumerable.");
        }
        if (desc.configurable) {
            $ERROR("The length property of a built-in function must not be configurable.");
        }
    }

    properties.forEach(function(prop) {
        var desc = Object.getOwnPropertyDescriptor(obj, prop);
        if (desc === undefined) {
            $ERROR("Missing property " + prop + ".");
        }
        
        if (desc.hasOwnProperty("writable") && !desc.writable) {
            $ERROR("The " + prop + " property of this built-in function must be writable.");
        }
        if (desc.enumerable) {
            $ERROR("The " + prop + " property of this built-in function must not be enumerable.");
        }
        if (!desc.configurable) {
            $ERROR("The " + prop + " property of this built-in function must be configurable.");
        }
    });

    
    
    
    
    
    var exception;
    if (isFunction && !isConstructor) {
        
        
        
        
        try {
            
            var instance = new obj();
        } catch (e) {
            exception = e;
        }
        if (exception === undefined || exception.name !== "TypeError") {
            $ERROR("Built-in functions that aren't constructors must throw TypeError when " +
                "used in a \"new\" statement.");
        }
    }

    if (isFunction && !isConstructor && obj.hasOwnProperty("prototype")) {
        $ERROR("Built-in functions that aren't constructors must not have a prototype property.");
    }

    
    return true;
}

