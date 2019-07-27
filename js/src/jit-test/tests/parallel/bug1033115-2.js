


if (!getBuildConfiguration().parallelJS)
  quit(0);

x = [];
y = x.mapPar(function() {});
Object.defineProperty(y, 3, {
    get: (function( ... $8)  {
        try {
            new Int8Array(y);
            x.reducePar(function() function q() 1);
        } catch (e) {}
    })
});
var x = [1,2,3];
function rsh() {
 return y.toSource() >> 2.0;
}
rsh();
