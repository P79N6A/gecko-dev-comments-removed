


var g = newGlobal('new-compartment');
var dbg = new Debugger(g);



function ApplyToFrameScript(code, skip, f) {
    dbg.onDebuggerStatement = function (frame) {
        while (skip-- > 0)
            frame = frame.older;
        assertEq(frame.type, "call");
        f(frame.script);
    };
    g.eval(code);
}

var savedScript;

ApplyToFrameScript('(function () { debugger; })();', 0,
                   function (script) {
                       assertEq(script instanceof Debugger.Script, true);
                       assertEq(script.live, true);
                       savedScript = script;
                   });
assertEq(savedScript.live, true);






