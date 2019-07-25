







assertEq(testLenientAndStrict("eval('010')",
                              completesNormally,
                              raisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict("eval('\"use strict\"; 010')",
                              raisesException(SyntaxError),
                              raisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict("var evil=eval; evil('010')",
                              completesNormally,
                              completesNormally),
         true);





assertEq(completesNormally("Function('010')"),
         true);
assertEq(raisesException(SyntaxError)("Function('\"use strict\"; 010')"),
         true);





var call_this, eval_this;
function f(code) {
  




  eval(code);
  call_this = this; 
}
f.call(true, 'eval_this = this');
assertEq(call_this, eval_this);

reportCompare(true, true);
