load(libdir + 'bytecode-cache.js');
var test = "";
var checkAfter;


test = (function () {
  function f(x) {
    function ifTrue() {
      return true;
    };
    function ifFalse() {
      return false;
    };

    if (x) return ifTrue();
    else return ifFalse();
  }

  return f.toSource() + "; f(true)";
})();
evalWithCache(test, { assertEqBytecode: true, assertEqResult : true });


test = (function () {
  function f(x) {
    function ifTrue() {
      return true;
    };
    function ifFalse() {
      return false;
    };

    if (x) return ifTrue();
    else return ifFalse();
  }

  return f.toSource() + "; f((generation % 2) == 0)";
})();
evalWithCache(test, { });


test = (function () {
  function f() {
    var upvar = "";
    function g() { upvar += ""; return upvar; }
    return g;
  }

  return f.toSource() + "; f()();";
})();
evalWithCache(test, { assertEqBytecode: true, assertEqResult : true });


test = (function () {
  function f() {
    var upvar = "";
    function g() { upvar += ""; return upvar; }
    return g;
  }

  return f.toSource() + "; f();";
})();
evalWithCache(test, { assertEqBytecode: true });


test = (function () {
  return "(" + (function () {
    p = function () {
        Set()
    };
    var Set = function () {};
    for (var x = 0; x < 5; x++) {
      Set = function (z) {
        return function () {
          [z]
        }
      } (x)
    }
  }).toSource() + ")()";
})();
evalWithCache(test, { assertEqBytecode: true });


test = (function () {
  function f() {
    var g = (a) => a + a;
    return g;
  }

  return f.toSource() + "; f()(1);";
})();
evalWithCache(test, { assertEqBytecode: true, assertEqResult : true });


test = (function () {
  function f() {
    var g = (a) => a + a;
    return g;
  }

  return f.toSource() + "; f();";
})();
evalWithCache(test, { assertEqBytecode: true });



gczeal(0);


test = "function f() { }; f();"
     + "assertEq(isLazyFunction(f), false);"
     + "var expect = isRelazifiableFunction(f);";
checkAfter = function (ctx) {
  gc(ctx.global.f); 
  evaluate("assertEq(isLazyFunction(f), expect);", ctx);
};
evalWithCache(test, {
  assertEqBytecode: true,  
  assertEqResult: true,    
                           
  checkAfter: checkAfter   
                           
});



test = "function f() { return isRelazifiableFunction(f) }; var expect = f();"
     + "assertEq(isLazyFunction(f), false);"
     + "expect";
checkAfter = function (ctx) {
  gc(ctx.global.f); 
  evaluate("assertEq(isLazyFunction(f), expect);", ctx);
};
evalWithCache(test, {
  assertEqBytecode: true,  
  assertEqResult: true,    
                           
  checkAfter: checkAfter   
                           
});
