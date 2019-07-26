


function f(a) {
   function g() {
       yield function () a;
   }
   return g();
}
var x;
f(7).next()();
