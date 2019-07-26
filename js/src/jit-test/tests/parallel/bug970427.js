


var pjs = getBuildConfiguration().parallelJS;
function f(a, b) {
  var o = a;
  gczeal(2);
}
f(3, 4);
var len = 5000;
var iters = 100;
for (var i = 0; i < iters; i++) {
  var par = pjs ? Array.buildPar(len, fill) : true;
}
function fill(i) { return 10/i; }
