





var p2 = new ParallelArray([2,2], function(i,j) { return i+j; });
p2.get({ 0: 1, 1: 0, testGet: 2 })
