
if (!("oomAfterAllocations" in this))
    quit();
var g = newGlobal("ar-u-nu-arab", this);
function attach(g, i) {
    var dbg = Debugger(g);
    oomAfterAllocations(10);
}
for (var i = 0; i < 3; i++)
    attach(g, i);
