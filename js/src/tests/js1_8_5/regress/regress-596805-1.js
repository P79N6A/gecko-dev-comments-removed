





var f = function(){};
for (var y in f);
f.j = 0;
Object.defineProperty(f, "k", ({configurable: true}));
delete f.j;
Object.defineProperty(f, "k", ({get: function() {}}));

reportCompare(0, 0, "ok");
