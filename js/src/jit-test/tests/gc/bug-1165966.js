
load(libdir + 'oomTest.js');
var g = newGlobal();
oomTest(function() {
    Debugger(g);
});
