
var f = (function () {with ({}) {}});
dis(f);
trap(f, 5, ''); 
f();
