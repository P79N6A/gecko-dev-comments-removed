

var global = newGlobal();
Debugger(global).onDebuggerStatement = function (frame) {
    var script = frame.eval("f").return.script;
    script.getAllColumnOffsets().forEach(function (offset) {
        script.setBreakpoint(offset.offset, {
            hit: function (frame) {
                assertEq(offset.lineNumber, 1);
                global.log += offset.columnNumber + " ";
            }
        });
    });
};

global.log = "";


global.eval("function f(){ 1 && print(print()) && new Error() } debugger;");
global.f();






assertEq(global.log, "14 25 19 37 48 ");
