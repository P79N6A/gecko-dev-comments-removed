

load(libdir + "iteration.js");


var map = Map();
var SIZE = 7;
for (var j = 0; j < SIZE; j++)
    map.set(j, j);


var NITERS = 5;
var iters = [];
for (var i = 0; i < NITERS; i++) {
    var iter = map[std_iterator]();
    assertIteratorResult(iter.next(), [0, 0], false);
    assertIteratorResult(iter.next(), [1, 1], false);
    iters[i] = iter;
}


for (var j = 0; j < SIZE; j += 2)
    map.delete(j);


for (var i = 0; i < NITERS; i++) {
    var iter = iters[i];
    for (var j = 3; j < SIZE; j += 2)
        assertIteratorResult(iter.next(), [j, j], false);
    assertIteratorResult(iter.next(), undefined, true);
}
