

let g = newGlobal('new-compartment');
let dbg = new Debugger;
let gw = dbg.addDebuggee(g);

function getSourceMapURL() {
    let fw = gw.makeDebuggeeValue(g.f);
    return fw.script.sourceMapURL;
}


g.evaluate("function f(x) { return 2*x; }");
assertEq(getSourceMapURL(), null);


g.evaluate("function f(x) { return 2*x; }", {sourceMapURL: 'file:///var/foo.js.map'});
assertEq(getSourceMapURL(), 'file:///var/foo.js.map');


let fired = false;
dbg.onDebuggerStatement = function (frame) {
    fired = true;
    assertEq(frame.script.sourceMapURL, 'file:///var/bar.js.map');
};
g.evaluate('(function () { (function () { debugger; })(); })();',
           {sourceMapURL: 'file:///var/bar.js.map'});
assertEq(fired, true);


g.evaluate('function f() {}\n' +
           '//@ sourceMappingURL=file:///var/quux.js.map');
assertEq(getSourceMapURL(), 'file:///var/quux.js.map');

g.evaluate('function f() {}\n' +
           '/*//@ sourceMappingURL=file:///var/quux.js.map*/');
assertEq(getSourceMapURL(), 'file:///var/quux.js.map');

g.evaluate('function f() {}\n' +
           '/*\n' +
           '//@ sourceMappingURL=file:///var/quux.js.map\n' +
           '*/');
assertEq(getSourceMapURL(), 'file:///var/quux.js.map');



g.evaluate('function f() {}\n' +
           '//@ sourceMappingURL=http://example.com/has illegal spaces.map');
assertEq(getSourceMapURL(), 'http://example.com/has');



g.evaluate('function f() {}\n' +
           '//@ sourceMappingURL=\n' +
           'function z() {}');
assertEq(getSourceMapURL(), null);
assertEq('z' in g, true);



g.evaluate('function f() {}\n' +
           '//@ sourceMappingURL=http://example.com/foo.js.map\n' +
           '//@ sourceMappingURL=http://example.com/bar.js.map');
assertEq(getSourceMapURL(), 'http://example.com/bar.js.map');


g.evaluate('function f() {}\n' +
           '//@ sourceMappingURL=http://example.com/foo.js.map',
           {sourceMapURL: 'http://example.com/bar.js.map'});
assertEq(getSourceMapURL(), 'http://example.com/foo.js.map');
