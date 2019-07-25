















function testLenientAndStrict(code, lenient_pred, strict_pred) {
  return (strict_pred("'use strict'; " + code) && 
          lenient_pred(code));
}





function completesNormally(code) {
  try {
    eval(code);
    return true;
  } catch (exception) {
    return false;
  }
}






function returns(value) {
  return function(code) {
    try {
      return eval(code) === value;
    } catch (exception) {
      return false;
    }
  }
}






function returnsCopyOf(value) {
  return function(code) {
    try {
      return deepEqual(eval(code), value);
    } catch (exception) {
      return false;
    }
  }
}







function raisesException(exception) {
  return function (code) {
    try {
      eval(code);
      return false;
    } catch (actual) {
      return actual instanceof exception;
    }
  };
};





function parsesSuccessfully(code) {
  try {
    Function(code);
    return true;
  } catch (exception) {
    return false;
  }
};





function parseRaisesException(exception) {
  return function (code) {
    try {
      Function(code);
      return false;
    } catch (actual) {
      return actual instanceof exception;
    }
  };
};






function clean_uneval(val) {
  return uneval(val).replace(/\s+/g, ' ');
}







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
