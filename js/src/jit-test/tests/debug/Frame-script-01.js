


var g = newGlobal('new-compartment');
var dbg = new Debugger(g);



function ApplyToFrameScript(code, skip, f) {
    dbg.hooks = {
        debuggerHandler: function (frame) {
            while (skip-- > 0)
                frame = frame.older;
            assertEq(frame.type, "eval");
            f(frame.script);
        }
    };
    g.eval(code);
}

var savedScript;

ApplyToFrameScript('debugger;', 0,
                   function (script) {
                       assertEq(script instanceof Debugger.Script, true);
                       assertEq(script.live, true);
                       savedScript = script;
                   });
assertEq(savedScript.live, false);
ApplyToFrameScript("(function () { eval('debugger;'); })();", 0,
                   function (script) {
                       assertEq(script instanceof Debugger.Script, true);
                       assertEq(script.live, true);
                       savedScript = script;
                   });
assertEq(savedScript.live, false);
