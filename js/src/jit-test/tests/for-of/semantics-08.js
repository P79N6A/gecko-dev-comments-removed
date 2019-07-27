

load(libdir + "iteration.js");

var g = newGlobal();
var it = g.eval("({ '" + std_iterator + "': function () { return this; }, " +
                "next: function () { return { done: true } } });");
for (x of it)
    throw 'FAIL';
