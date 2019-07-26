

var g = newGlobal();
var dbg = new Debugger();

g.eval("// Header comment\n" +   
       "\n" +
       "\n" +
       "function f(n) {\n" +     
       "    var foo = '!';\n" +
       "}");

dbg.addDebuggee(g);
var scripts = dbg.findScripts();
var found = false;
for (var i = 0; i < scripts.length; i++) {
  found = found || scripts[i].startLine == 6;
  
  
  assertEq(scripts[i].getLineOffsets(9).length, 0);
}
assertEq(found, true);
