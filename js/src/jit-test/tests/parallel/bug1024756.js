

if (!getBuildConfiguration().parallelJS)
  quit(0);

var x =
(function() {
    return Array.buildPar(15891, function() {
        return [].map(function() {})
    })
})();
assertEq(x.length, 15891);
