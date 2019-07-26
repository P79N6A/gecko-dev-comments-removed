

var g = newGlobal();
var it = g.eval("({ iterator: function () { return this; }, " +
                "next: function () { throw StopIteration; } });");
for (x of it)
    throw 'FAIL';
