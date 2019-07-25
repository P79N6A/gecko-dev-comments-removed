















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
      return exception.prototype.isPrototypeOf(actual);
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
      return exception.prototype.isPrototypeOf(actual);
    }
  };
};






function clean_uneval(val) {
  return uneval(val).replace(/\s+/g, ' ');
}
