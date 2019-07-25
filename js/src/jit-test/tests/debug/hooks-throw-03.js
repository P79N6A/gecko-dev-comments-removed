


var g = newGlobal('new-compartment');
g.debuggeeGlobal = this;
g.dbg = null;
g.eval("(" + function () {
        dbg = new Debug(debuggeeGlobal);
        dbg.hooks = {
            throw: function (frame, exc) {
                assertEq(frame instanceof Debug.Frame, true);
                assertEq(exc instanceof Debug.Object, true);
                var s = '!';
                for (var f = frame; f; f = f.older)
                    if (f.type === "call")
                        s += f.callee.name;
                s += ', ';
                debuggeeGlobal.log += s;
            }
        };
    } + ")();");

var log;

function k() {
    try {
        throw new Error("oops");  
    } finally {
        log += 'k-finally, ';
    } 
}

function j() {
    k();  
    log += 'j-unreached, ';
}

function h() {
    try {
        j();  
        log += 'h-unreached, ';
    } catch (exc) {
        log += 'h-catch, ';
        throw exc; 
    }
}

function f() {
    try {
        h(); 
    } catch (exc) {
        log += 'f-catch, ';
    }
    log += 'f-after, ';
}

log = '';
f();
g.dbg.enabled = false;
assertEq(log, '!kjhf, k-finally, !kjhf, !jhf, !hf, h-catch, !hf, !f, f-catch, f-after, ');
