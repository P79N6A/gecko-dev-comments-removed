






function isSyntaxError(code) {
  try {
    eval(code);
    return false;
  } catch (exception) {
    if (SyntaxError.prototype.isPrototypeOf(exception))
      return true;
    throw exception;
  };
};







assertEq(isSyntaxError("function f(x,x){}"),                false);

assertEq(isSyntaxError("function f(x,[x]){})"),             true);
assertEq(isSyntaxError("function f(x,{y:x}){})"),           true);
assertEq(isSyntaxError("function f(x,{x}){})"),             true);

assertEq(isSyntaxError("function f([x],x){})"),             true);
assertEq(isSyntaxError("function f({y:x},x){})"),           true);
assertEq(isSyntaxError("function f({x},x){})"),             true);

assertEq(isSyntaxError("function f([x,x]){}"),              true);
assertEq(isSyntaxError("function f({x,x}){}"),              true);
assertEq(isSyntaxError("function f({y:x,z:x}){}"),          true);

assertEq(isSyntaxError("function f(x,x,[y]){}"),            true);
assertEq(isSyntaxError("function f(x,x,{y}){}"),            true);
assertEq(isSyntaxError("function f([y],x,x){}"),            true);
assertEq(isSyntaxError("function f({y},x,x){}"),            true);

assertEq(isSyntaxError("function f(a,b,c,d,e,f,g,h,b,[y]){}"),  true);
assertEq(isSyntaxError("function f([y],a,b,c,d,e,f,g,h,a){}"),  true);
assertEq(isSyntaxError("function f([a],b,c,d,e,f,g,h,i,a){}"),  true);
assertEq(isSyntaxError("function f(a,b,c,d,e,f,g,h,i,[a]){}"),  true);
