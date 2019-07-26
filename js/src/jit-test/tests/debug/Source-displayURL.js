


let g = newGlobal('new-compartment');
let dbg = new Debugger;
let gw = dbg.addDebuggee(g);

function getDisplayURL() {
    let fw = gw.makeDebuggeeValue(g.f);
    return fw.script.source.displayURL;
}


g.evaluate("function f(x) { return 2*x; }");
assertEq(getDisplayURL(), null);


g.evaluate("function f(x) { return 2*x; }", {sourceURL: 'file:///var/foo.js'});
assertEq(getDisplayURL(), 'file:///var/foo.js');


let fired = false;
dbg.onDebuggerStatement = function (frame) {
    fired = true;
    assertEq(frame.script.source.displayURL, 'file:///var/bar.js');
};
g.evaluate('(function () { (function () { debugger; })(); })();',
           {sourceURL: 'file:///var/bar.js'});
assertEq(fired, true);


g.evaluate('function f() {}\n' +
           '//# sourceURL=file:///var/quux.js');
assertEq(getDisplayURL(), 'file:///var/quux.js');

g.evaluate('function f() {}\n' +
           '/*//# sourceURL=file:///var/quux.js*/');
assertEq(getDisplayURL(), 'file:///var/quux.js');

g.evaluate('function f() {}\n' +
           '/*\n' +
           '//# sourceURL=file:///var/quux.js\n' +
           '*/');
assertEq(getDisplayURL(), 'file:///var/quux.js');



g.evaluate('function f() {}\n' +
           '//# sourceURL=http://example.com/has illegal spaces');
assertEq(getDisplayURL(), 'http://example.com/has');



g.evaluate('function f() {}\n' +
           '//# sourceURL=\n' +
           'function z() {}');
assertEq(getDisplayURL(), null);
assertEq('z' in g, true);


g.evaluate('function f() {}\n' +
           '//# sourceURL=http://example.com/foo.js\n' +
           '//# sourceURL=http://example.com/bar.js');
assertEq(getDisplayURL(), 'http://example.com/bar.js');


g.evaluate('function f() {}\n' +
           '//# sourceURL=http://example.com/foo.js',
           {sourceMapURL: 'http://example.com/bar.js'});
assertEq(getDisplayURL(), 'http://example.com/foo.js');

