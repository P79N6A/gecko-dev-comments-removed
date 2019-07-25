


var global = newGlobal('new-compartment');
global.eval('function f() { debugger; }');
global.eval('function g() { debugger; }');

var debug = new Debug(global);

function evalAndNoteScripts(prog) {
    var scripts = {};
    debug.hooks = {
        debuggerHandler: function(frame) {
            if (frame.type == "call")
                assertEq(frame.script, frame.callee.script);
            scripts.frame = frame.script;
            if (frame.arguments[0])
                scripts.argument = frame.arguments[0].script;
        }
    };
    global.eval(prog);
    return scripts;
}



var scripts = evalAndNoteScripts('f(f)');
assertEq(scripts.frame, scripts.argument);
var fScript = scripts.argument;


scripts = evalAndNoteScripts('f(f)');
assertEq(scripts.frame, fScript);
assertEq(scripts.argument, fScript);


scripts = evalAndNoteScripts('f(g)');
assertEq(scripts.frame !== scripts.argument, true);
assertEq(scripts.frame, fScript);
var gScript = scripts.argument;


scripts = evalAndNoteScripts('g(f)');
assertEq(scripts.frame !== scripts.argument, true);
assertEq(scripts.frame,    gScript);
assertEq(scripts.argument, fScript);
